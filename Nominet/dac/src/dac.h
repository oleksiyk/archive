/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#ifndef DAC_H
#define DAC_H

#include "decl.h"

class Dac
{
private:
    Log * log;
    DacConfig * config;
    char * pidfile;

    static int flag_restart, flag_shutdown;

    int sockfd;

    const char * configFileName;

    static const char * requiredConfigOptions[];

    DacThread * readThread;

private:
    int checkPIDFile();
    int writePIDFile();
    int setUserAndGroupID();

    int initsocket();

    int doqueries();

    static void termination_signal_handler(int);
    static void restart_signal_handler(int);

    int restart();

public:
    Dac(const char * _configFileName);
    virtual ~Dac();

    const Log * getLog()const{return log;}
    const DacConfig * getConfig()const{return config;}

    int initialize();
    int start();
};

#endif
