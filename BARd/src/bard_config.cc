#include "bard_config.h"

BARdConfig::BARdConfig(const Log * _log):
    log(_log),
    error_log_file(NULL),error_log_level(0),
    barcode_pid_file(NULL), barcode_output_file(NULL),
    signal_app_pid_file(NULL), signal_app_path(NULL),
    signal(SIGUSR2)
{
}

BARdConfig::~BARdConfig()
{
    free(error_log_file);
    free(barcode_pid_file);
    free(barcode_output_file);
    free(signal_app_pid_file);
    free(signal_app_path);
}

int BARdConfig::initialize(const char * _configFileName)
{
    xmlNodePtr cur;
    
    xmlKeepBlanksDefault(0);

    /*
    error_log_file = NULL;
    error_log_level = 0;
    barcode_pid_file = NULL;
    barcode_output_file = NULL;
    signal_app_pid_file = NULL;
    signal_app_path = NULL;
    signal = SIGUSR2;
    */

    configDoc = xmlParseFile(_configFileName);
    if (configDoc == NULL) {
        log->error(0, "Couldn't open config file '%s'", _configFileName);
        return -1;
    }

    cur = xmlDocGetRootElement(configDoc);
    if (cur == NULL) {
        xmlFreeDoc(configDoc);
        log->error(0, "Empty document (config file)");
        return -1;
    }

    if ((xmlStrcmp(cur->name, (const xmlChar *) "BARd"))) {
        log->error(0, "%s is not BARd configuration file", _configFileName);
        return -1;
    }
    
    cur = cur->xmlChildrenNode;

    while(cur) {
        if(cur->type == XML_COMMENT_NODE ){
            cur  = cur->next;
            continue;
        }
        if (!strcasecmp("BARCODE_PID_FILE", (const char *) cur->name)) {
            barcode_pid_file = (char *) xmlNodeListGetString(configDoc, cur->xmlChildrenNode, 1);
        } else if (!strcasecmp("BARCODE_OUTPUT_FILE", (const char *) cur->name)) {
            barcode_output_file = (char*) xmlNodeListGetString(configDoc, cur->xmlChildrenNode, 1);
        } else if (!strcasecmp("BARCODE_ERROR_LOG_FILE", (const char *) cur->name)) {
            error_log_file = (char*) xmlNodeListGetString(configDoc, cur->xmlChildrenNode, 1);
        } else if (!strcasecmp("SIGNAL_APP_PID_FILE", (const char *) cur->name)) {
            signal_app_pid_file = (char*) xmlNodeListGetString(configDoc, cur->xmlChildrenNode, 1);
        } else if (!strcasecmp("SIGNAL_APP_PATH", (const char *) cur->name)) {
            signal_app_path = (char*) xmlNodeListGetString(configDoc, cur->xmlChildrenNode, 1);
        } else if (!strcasecmp("USE_SIGNAL", (const char *) cur->name)) {
            char * s = (char*) xmlNodeListGetString(configDoc, cur->xmlChildrenNode, 1);
            if(!strcasecmp("SIGHUP", s)){
                signal = SIGHUP;
            } else if(!strcasecmp("SIGINT", s)){
                signal = SIGINT;
            } else if(!strcasecmp("SIGKILL", s)){
                signal = SIGKILL;
            } else if(!strcasecmp("SIGALRM", s)){
                signal = SIGALRM;
            } else if(!strcasecmp("SIGUSR1", s)){
                signal = SIGUSR1;
            } else if(!strcasecmp("SIGUSR2", s)){
                signal = SIGUSR2;
            } else if(!strcasecmp("SIGCHLD", s)){
                signal = SIGCHLD;
            } else if(!strcasecmp("SIGCONT", s)){
                signal = SIGCONT;
            } else {
                log->warning(0, "unknown signal %s, will use SIGUSR2", s);
                signal = SIGUSR2;
            }
            free(s);
        }
        cur = cur->next;
    };
    
    
    xmlFreeDoc(configDoc);

    return 0;
}
