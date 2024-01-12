#include "shotgund_config.h"
#include "decl.h"

void ShotgundConfig::error(int lineNum, const char * fileName, const char * fmt, ...)
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

    log->vlog(0,0, buf, args);

    va_end(args);
}

int ShotgundConfig::checkConfig()
{
    const DOTCONFDocumentNode * node = findNode("debug_file");

    if(!node->getValue()){
        log->log(1, 0,0, "file %s, line %d: parameter debug_file must have value",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

    node = findNode("async_query");

    if(node != NULL){
        if(node->getValue()){
            if(!strcasecmp(node->getValue(), "off")){
                async_query = 0;
            }
        } else {
            log->log(1, 0,0, "file %s, line %d: parameter async_query must have value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
    }

    node = findNode("debug");

    if(!node->getValue()){
        log->log(1, 0,0, "file %s, line %d: parameter debug must have value",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

    int debug = atoi(node->getValue());

    if(debug < 1 || debug > 4){
        log->log(1, 0,0, "file %s, line %d: debug value must be between 0..4",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

    node = findNode("pid_file");

    if(node->getValue() == NULL){
        log->log(1, 0,0, "file %s, line %d: parameter pid_file must have value",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

    node = findNode("user");

    if(node){
        if(node->getValue() == NULL){
            log->log(1, 0,0, "file %s, line %d: parameter user must have value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
    }

    node = findNode("port");
    if(node){
        if(node->getValue() == NULL){
            log->log(1, 0,0, "file %s, line %d: parameter port must have value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
    }

    node = NULL;
    while((node = findNode("host", NULL, node)) != NULL){
#ifdef CRIPPLED
        const char * qtype = node->getValue(2);
        if(strcasecmp(qtype, "NAPTR")){
            log->log(1,0,0,"warning: file %s, line %d: host line IGNORED - not NAPTR type",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
        }
#endif
        const char * rule = node->getValue(3);
        if(!rule){
            log->log(1,0,0,"file %s, line %d: parameter host: invalid syntax",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
                return -1;
        }
        if(strcasecmp(rule, "permit") && strcasecmp(rule, "deny")){
            log->log(1,0,0,"file %s, line %d: parameter host: invalid syntax",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
        int i = 4;
        const char * s = NULL;
        bool ioption = false, joption = false;
        while((s = node->getValue(i++)) != NULL){
            if(s[0] == 'i' && s[1] == 0){ ioption = true; continue ;}
            if(s[0] == 'j' && s[1] == 0){ joption = true; continue; }

            const DOTCONFDocumentNode * znode = NULL;
            int zcount = 0;
            while((znode = findNode("zone", NULL, znode)) != NULL){
                if(znode->getValue() == NULL){
                    log->log(1,0,0,"file %s, line %d: zone line syntax error",
                        znode->getConfigurationFileName(), znode->getConfigurationLineNumber());
                    return -1;
                }
                if(znode->getValue(4) == NULL){
                    log->log(1,0,0,"file %s, line %d: zone line syntax error",
                        znode->getConfigurationFileName(), znode->getConfigurationLineNumber());
                    return -1;
                }
                if(strcmp(znode->getValue(), s)) continue;
#ifdef CRIPPLED
                if(zcount){
                    log->log(1,0,0,"warning: file %s, line %d: zone line will be IGNORED",
                        znode->getConfigurationFileName(), znode->getConfigurationLineNumber());
                }
#endif
                zcount++;
            }
/*
            const DOTCONFDocumentNode * fnode = NULL;
            int fcount = 0;
            while((fnode = findNode("forwarder", NULL, fnode)) != NULL){
                if(fnode->getValue() == NULL){
                    log->log(1,0,0,"file %s, line %d: forwarder line syntax error",
                        fnode->getConfigurationFileName(), fnode->getConfigurationLineNumber());
                    return -1;
                }
                if(strcmp(fnode->getValue(), s)) continue;
#ifdef CRIPPLED
                if(fcount){
                    log->log(1,0,0,"warning: file %s, line %d: forwarder line will be IGNORED",
                        fnode->getConfigurationFileName(), fnode->getConfigurationLineNumber());
                }
#endif
                fcount++;
            }
*/

        }
        if(ioption && joption){
            log->log(1,0,0,"file %s, line %d: parameter host: both 'i' and 'j' options provided, please choose one",
                    node->getConfigurationFileName(), node->getConfigurationLineNumber(), s);
                return -1;
        }

        if((i-5) < 1 && strcasecmp(rule, "deny")){
            log->log(1,0,0,"file %s, line %d: parameter host: no groups defined for permit operation",
                    node->getConfigurationFileName(), node->getConfigurationLineNumber(), MAX_PAR);
                return -1;
        }

        if((i-4) > MAX_PAR){
            log->log(1,0,0,"file %s, line %d: parameter host: too much groups defined, max is %d",
                    node->getConfigurationFileName(), node->getConfigurationLineNumber(), MAX_PAR);
                return -1;
        }
    }

    return 0;

}
