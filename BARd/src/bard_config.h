#ifndef BARD_CONFIG_H
#define BARD_CONFIG_H

#include "config.h"
#include "log.h"
#include "bard_sys.h"
#include "bard_signal.h"

#include <libxml/tree.h>

class BARdConfig
{
private:
    const Log * log;
    xmlDocPtr configDoc;

private:
    char * error_log_file;
    int error_log_level;
    char * barcode_pid_file;
    char * barcode_output_file;
    char * signal_app_pid_file;
    char * signal_app_path;
    int signal;

public:
    BARdConfig(const Log * _log);
    virtual ~BARdConfig();

    int initialize(const char * _configFileName);

public:
    const char * getErrorLogFile()const{ return error_log_file;}
    const char * getBarcodePIDFile()const{return barcode_pid_file;}
    const char * getBarcodeOutputFile()const{return barcode_output_file;}
    const char * getSignalAppPIDFile()const{return signal_app_pid_file;}
    const char * getSignalAppPath()const{return signal_app_path;}
    int getSignal()const { return signal; }
    int getErrorLogLevel()const{ return error_log_level;}

};

#endif
