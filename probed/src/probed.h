/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_H
#define PROBED_H

#include "decl.h"

class Probed
{
private:
    Log * log;
    ProbedHistory * history;
    ProbedConfig * config;
    ProbedSSL * ssl;
    char * pidfile;

    static int flag_restart, flag_shutdown;

    int sockfd;
    int pingfd;

    const char * configFileName;

    static const char * requiredConfigOptions[];

    ProbedThreadManager * threadManager;
    ProbedPingThread * pingThread;
    ThreadQueue<ProbedPingTask*> * pingQueue;

private:
    int checkPIDFile();
    int writePIDFile();
    int setUserAndGroupID();

    int initsocket();
    int initPingSocket();

    static void termination_signal_handler(int);
    static void restart_signal_handler(int);

    int restart();

    int doQuery();

public:
    Probed(const char * _configFileName);
    virtual ~Probed();

    const Log * getLog()const{return log;}
    const ProbedHistory * getHistory()const{return history;}
    const ProbedConfig * getConfig()const{return config;}
    ThreadQueue<ProbedPingTask*> * getPingQueue()const{ return pingQueue;}

    int initialize();
    int start();

    SSL_CTX * getSSL_CTX() const;

    ProbedThreadManager * getThreadManager()const{ return threadManager;}
};

#endif
