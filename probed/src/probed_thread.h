/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_THREAD_H
#define PROBED_THREAD_H

#include "decl.h"
#include <memory>

class ProbedThread
{
private:
    const Log * log;
    struct pollfd fdset;
    char peerName[INET_ADDRSTRLEN + 10];

    ProbedSSLTransport * tr;

    bool shutdown;

    pthread_t threadSelf;

    int initialize();
    int process();
    int processCommands();
    static void * threadFunc(void * p);

public:
    ProbedThread(int _fd);
    virtual ~ProbedThread();

    struct pollfd * getfdset() { return &fdset; }

    const char * getPeerName() const{ return peerName; }

    int start();
    void stop();
};

#endif
