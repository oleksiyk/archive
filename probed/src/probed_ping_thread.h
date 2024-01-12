/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_PING_THREAD_H
#define PROBED_PING_THREAD_H

#include "decl.h"

#include <algorithm>

class ProbedPingThread
{
private:
    const Log * log;
    ThreadQueue<ProbedPingTask*> * queue;

    int pingfd;

    int tcpfd;
    struct timeval tcptv[2];

    pthread_t threadSelf;

    static void * threadFunc(void * p);
    static void threadCleanupFunc(void * p);

    int sendICMPPing(ProbedPingTask * obj);
    int recvICMPPing(ProbedPingTask * obj);
    int processICMPPacket(char * ptr, ProbedPingTask * obj, struct timeval * tvrecv, socklen_t len, const char * name);

    int sendTCPPing(ProbedPingTask * obj);
    int recvTCPPing(ProbedPingTask * obj);

    int calculateResults(ProbedPingTask * obj);

public:
    ProbedPingThread(ThreadQueue<ProbedPingTask*> * _queue, int _pingfd);
    virtual ~ProbedPingThread();

    int start();
    void stop();
};

#endif
