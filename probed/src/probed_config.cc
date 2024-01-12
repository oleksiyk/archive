/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#include "probed_config.h"
#include "decl.h"

void ProbedConfig::error(int lineNum, const char * fileName, const char * fmt, ...)
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

int ProbedConfig::checkConfig()
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

    node = findNode("listen");
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

    node = findNode("session_timeout");
    if(node){
        if(!node->getValue()){
            log->log(1, "file %s, line %d: parameter 'session_timeout' must have value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
    }

    node = findNode("max_packets");
    if(node){
        if(!node->getValue()){
            log->log(1, "file %s, line %d: parameter 'max_packets' must have value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
    }

    node = findNode("history_file");
    if(node){
        if(!node->getValue()){
            log->log(1, "file %s, line %d: parameter 'history_file' must have value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
    }

    node = findNode("type");
    if(node){
        if(!node->getValue()){
            log->log(1, "file %s, line %d: parameter 'type' must have value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
        if(!strcasecmp(node->getValue(), "ECHO")){
            _cmdTCP = 1;
            node = findNode("echo_string");
            if(!node){
                log->log(1, "TYPE is set to ECHO but echo_string parameter is missing");
                return -1;
            }
            if(!node->getValue()){
                log->log(1, "file %s, line %d: parameter 'echo_string' must have value",
                    node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }

    if(parseThresholds() == -1){
        return -1;
    }

    if(parseAccessLines() == -1){
        return -1;
    }

    return 0;

}

int ProbedConfig::parseThresholds()
{
    const DOTCONFDocumentNode * node = NULL;

    int excellent = 5;
    int good = 4;
    int fair = 3;
    int poor = 2;
    int bad = 1;

    node = findNode("excellent");
    if(node){
        if(node->getValue()){
            excellent = atoi(node->getValue());
        }
    }
    node = findNode("good");
    if(node){
        if(node->getValue()){
            good = atoi(node->getValue());
        }
    }
    node = findNode("fair");
    if(node){
        if(node->getValue()){
            fair = atoi(node->getValue());
        }
    }
    node = findNode("poor");
    if(node){
        if(node->getValue()){
            poor = atoi(node->getValue());
        }
    }
    node = findNode("bad");
    if(node){
        if(node->getValue()){
            bad = atoi(node->getValue());
        }
    }

    for(int i=0;i<5;i++){
        latency_thresholds[i][0] = -1;
        latency_thresholds[i][1] = -1;
        latency_thresholds[i][2] = -1;
        jitter_thresholds[i][0] = -1;
        jitter_thresholds[i][1] = -1;
        jitter_thresholds[i][2] = -1;
        loss_thresholds[i][0] = -1;
        loss_thresholds[i][1] = -1;
        loss_thresholds[i][2] = -1;
    }

    const char * v = NULL;

    // ugly code, i know :)

    node = findNode("latency_excellent");
    if(node){
        v = node->getValue();
        if(v){
            latency_thresholds[0][0] = getMin(v);
            latency_thresholds[0][1] = getMax(v);
            latency_thresholds[0][2] = excellent;
            if(latency_thresholds[0][0] == float(-1) || latency_thresholds[0][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("latency_good");
    if(node){
        v = node->getValue();
        if(v){
            latency_thresholds[1][0] = getMin(v);
            latency_thresholds[1][1] = getMax(v);
            latency_thresholds[1][2] = good;
            if(latency_thresholds[1][0] == float(-1) || latency_thresholds[1][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("latency_fair");
    if(node){
        v = node->getValue();
        if(v){
            latency_thresholds[2][0] = getMin(v);
            latency_thresholds[2][1] = getMax(v);
            latency_thresholds[2][2] = fair;
            if(latency_thresholds[2][0] == float(-1) || latency_thresholds[2][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("latency_poor");
    if(node){
        v = node->getValue();
        if(v){
            latency_thresholds[3][0] = getMin(v);
            latency_thresholds[3][1] = getMax(v);
            latency_thresholds[3][2] = poor;
            if(latency_thresholds[3][0] == float(-1) || latency_thresholds[3][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("latency_bad");
    if(node){
        v = node->getValue();
        if(v){
            latency_thresholds[4][0] = getMin(v);
            latency_thresholds[4][1] = getMax(v);
            latency_thresholds[4][2] = bad;
            if(latency_thresholds[4][0] == float(-1) || latency_thresholds[4][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }

    // JITTER

    node = findNode("jitter_excellent");
    if(node){
        v = node->getValue();
        if(v){
            jitter_thresholds[0][0] = getMin(v);
            jitter_thresholds[0][1] = getMax(v);
            jitter_thresholds[0][2] = excellent;
            if(jitter_thresholds[0][0] == float(-1) || jitter_thresholds[0][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("jitter_good");
    if(node){
        v = node->getValue();
        if(v){
            jitter_thresholds[1][0] = getMin(v);
            jitter_thresholds[1][1] = getMax(v);
            jitter_thresholds[1][2] = good;
            if(jitter_thresholds[1][0] == float(-1) || jitter_thresholds[1][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("jitter_fair");
    if(node){
        v = node->getValue();
        if(v){
            jitter_thresholds[2][0] = getMin(v);
            jitter_thresholds[2][1] = getMax(v);
            jitter_thresholds[2][2] = fair;
            if(jitter_thresholds[2][0] == float(-1) || jitter_thresholds[2][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("jitter_poor");
    if(node){
        v = node->getValue();
        if(v){
            jitter_thresholds[3][0] = getMin(v);
            jitter_thresholds[3][1] = getMax(v);
            jitter_thresholds[3][2] = poor;
            if(jitter_thresholds[3][0] == float(-1) || jitter_thresholds[3][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("jitter_bad");
    if(node){
        v = node->getValue();
        if(v){
            jitter_thresholds[4][0] = getMin(v);
            jitter_thresholds[4][1] = getMax(v);
            jitter_thresholds[4][2] = bad;
            if(jitter_thresholds[4][0] == float(-1) || jitter_thresholds[4][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }

    // LOSS

    node = findNode("loss_excellent");
    if(node){
        v = node->getValue();
        if(v){
            loss_thresholds[0][0] = getMin(v);
            loss_thresholds[0][1] = getMax(v);
            loss_thresholds[0][2] = excellent;
            if(loss_thresholds[0][0] == float(-1) || loss_thresholds[0][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("loss_good");
    if(node){
        v = node->getValue();
        if(v){
            loss_thresholds[1][0] = getMin(v);
            loss_thresholds[1][1] = getMax(v);
            loss_thresholds[1][2] = good;
            if(loss_thresholds[1][0] == float(-1) || loss_thresholds[1][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("loss_fair");
    if(node){
        v = node->getValue();
        if(v){
            loss_thresholds[2][0] = getMin(v);
            loss_thresholds[2][1] = getMax(v);
            loss_thresholds[2][2] = fair;
            if(loss_thresholds[2][0] == float(-1) || loss_thresholds[2][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("loss_poor");
    if(node){
        v = node->getValue();
        if(v){
            loss_thresholds[3][0] = getMin(v);
            loss_thresholds[3][1] = getMax(v);
            loss_thresholds[3][2] = poor;
            if(loss_thresholds[3][0] == float(-1) || loss_thresholds[3][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }
    node = findNode("loss_bad");
    if(node){
        v = node->getValue();
        if(v){
            loss_thresholds[4][0] = getMin(v);
            loss_thresholds[4][1] = getMax(v);
            loss_thresholds[4][2] = bad;
            if(loss_thresholds[4][0] == float(-1) || loss_thresholds[4][1] == (float)-1){
                log->log(1, "file %s, line %d: thresholds parse error",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
            }
        }
    }

    return 0;
}

float ProbedConfig::getMin(const char * v)
{
    char * p = strchr(v, '(');
    if(!p){
        log->log(1, "1");
        return -1;
    }
    char * d = strchr(v, ',');
    if(!d){
        log->log(1, "2");
        return -1;
    }
    *d = 0;
    float f = atof(++p);
    *d = ',';
    return f;
}

float ProbedConfig::getMax(const char * v)
{
    char * p = strchr(v, ')');
    if(!p){
        log->log(1, "3");
        return -1;
    }
    *p = 0;
    char * d = strchr(v, ',');
    if(!d){
        log->log(1, "4");
        return -1;
    }
    float f = atof(++d);
    *p = ')';
    return f;
}

int ProbedConfig::parseNetworkAddress(const char *str, u_int32_t *addr, u_int32_t *mask)
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

int ProbedConfig::parseHostLine(const DOTCONFDocumentNode * node, u_int32_t *addr)
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

int ProbedConfig::parseAccessLines()
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
