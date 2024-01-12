#ifndef SHOTGUND_H
#define SHOTGUND_H

#include "decl.h"

#include "shotgund_signal.h"
#include "shotgund_config.h"

#include <getopt.h>

class Shotgund
{
public:
    typedef ThreadManager<ThreadQueue<QueueObj*>,ResolverThread> ThreadManager;

private:
    Log * log;
    ShotgundConfig * config;
    char * pidfile;

    static int flag_restart, flag_shutdown;

    struct pollfd fds[8];

    const char * configFileName;

    static const char * requiredConfigOptions[];

    ThreadQueue<QueueObj*> * queue;
    ThreadManager * threadManager;

private:
    int checkPIDFile();
    int writePIDFile();
    int setUserAndGroupID();
    u_int16_t getRandId();

    int initsockets();

    static void termination_signal_handler(int);
    static void restart_signal_handler(int);

    int restart();

public:
    Shotgund(const char * _configFileName);
    virtual ~Shotgund();

    const Log * getLog()const{return log;}
    const ShotgundConfig * getConfig()const{return config;}

    int initialize();
    int start();

    const ThreadManager * getThreadManager()const{ return threadManager;}
};

#endif
