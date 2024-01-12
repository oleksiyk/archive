#ifndef BARD_H
#define BARD_H

#include "config.h"
#include "log.h"
#include "bard_sys.h"
#include "bard_signal.h"
#include "bard_config.h"
#include "bard_evdev.h"

#include <getopt.h>

#include <libxml/tree.h>

class BARd
{
private:
    Log * log;
    BARdConfig * config;
    
    static int flag_restart, flag_shutdown;

    BARdEvdev * devices[32];
    struct pollfd fds[32];
    FILE * output_fp;

    const char * configFileName;

private:    
    int checkPIDFile();
    int writePIDFile();
    /*
    int setUserAndGroupID();
    */
    static void termination_signal_handler(int);
    static void restart_signal_handler(int);

    int openReaders();
    int pingMainApp();
    pid_t searchPID();

    int restart();
    
public:
    BARd(const char * _configFileName);
    virtual ~BARd();

    int initialize();
    int start();
};

#endif
