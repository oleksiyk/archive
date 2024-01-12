/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#include "dac_config.h"
#include "decl.h"

void DacConfig::error(int lineNum, const char * fileName, const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    size_t len = (lineNum!=0?strlen(fileName):0) + strlen(fmt) + 50;

    char * buf = (char*)mempool->alloc(len);

    if(lineNum)
        (void) snprintf(buf, len, "file '%s', line %d: %s", fileName, lineNum, fmt);
    else
        (void) snprintf(buf, len, "%s", fmt);

    buf[len-1] = 0;

    log->vlog(buf, args);

    va_end(args);
}

int DacConfig::checkConfig()
{
    const DOTCONFDocumentNode * node = NULL;

    node = findNode("debug_level");

    if(!node->getValue()){
        log->log(1, "file %s, line %d: parameter debug_level must have value",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

    int debug = atoi(node->getValue());

    if(debug < 1 || debug > 9){
        log->log(1, "file %s, line %d: debug_level value must be between 0..9",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

    node = findNode("connect");
    if(node->getValue() == NULL){
        log->log(1, "file %s, line %d: 'host' parameter missing",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

    node = findNode("logfile");
    if(node){
        if(!node->getValue()){
            log->log(1, "file %s, line %d: parameter 'logfile' must have value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
    }

    return 0;

}

int DacConfig::parseNetworkAddress(const char *str, u_int32_t *addr, u_int32_t *mask)
{
    char o[4][4];

    *addr = (u_int32_t)(-1);
    *mask = (u_int32_t)(-1);

    int z = sscanf(str, "%3[*0-9].%3[*0-9].%3[*0-9].%3[*0-9]/%2d", o[0], o[1], o[2], o[3], mask);

    if(z<4){
        return -1;
    }
    if((z == 5) && (*mask <= 32)) {
        *mask = ((u_int32_t)-1) >> (32-*mask);
    }
    for(int i = 0; i<4; i++){
        if(*o[i] == '*'){
            if(o[i][1] != 0){
                return -1;
            }
            ((u_int8_t*)addr)[i] = 0;
            ((u_int8_t*)mask)[i] = 0;
        } else if(!strchr(o[i], '*')){
            if(atoi(o[i]) > 255){
                return -1;
            }
            ((u_int8_t*)addr)[i] = atoi(o[i]);
        } else {
            return -1;
        }
    }
    //printf("z=%d, o1=%s, o2=%s, o3=%s, o4=%s, addr=%d, mask=%d\n",  z, o[0], o[1], o[2], o[3], *addr, *mask);
    return 0;
}

int DacConfig::parseHostLine(const DOTCONFDocumentNode * node, u_int32_t *addr)
{
    const char * str = node->getValue();
    struct hostent * hep = gethostbyname(str);

    if ((!hep) || (hep->h_addrtype != AF_INET || !hep->h_addr_list[0])) {
        log->log(1, "file %s, line %d: Cannot resolve hostname '%s'",
                node->getConfigurationFileName(), node->getConfigurationLineNumber(), str);
        return -1;
    }

    *addr = ((struct in_addr *) (hep->h_addr))->s_addr;

    return 0;
}

int DacConfig::parseAccessLines()
{
    const DOTCONFDocumentNode * node = NULL;

    while((node = findNode("allow", NULL, node)) != NULL){
        const char * v = node->getValue();
        if(!v){
            log->log(1, "file %s, line %d: parameter 'allow' must have value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }

        struct ipms ipms;

        if(parseNetworkAddress(v, &ipms.addr, &ipms.mask) == -1){
            if(parseHostLine(node, &ipms.addr) == -1){
                log->log(1, "file %s, line %d: parameter 'allow' parse error",
                    node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            } else {
                ipms.mask = ~0;
            }
        }

        ipAllowList.push_back(ipms);

    }

    return 0;
}
