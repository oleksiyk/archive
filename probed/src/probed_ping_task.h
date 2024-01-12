/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_PING_TASK_H
#define PROBED_PING_TASK_H

#include "decl.h"
#include <list>

struct ProbedPingTask
{
    int packets;
    bool verbose;
    bool useTcp;
    struct sockaddr_in addr;

    const char * sslPeerName;

    std::list<float> rtts;

    int packets_recvd;
    float rtt_average;
    float rtt_min;
    float rtt_max;
    float jitter;

    pthread_mutex_t lock;
    pthread_cond_t cond;

    ProbedPingTask():
        packets(DEFAULT_PACKETS),
        verbose(false),
        packets_recvd(0),
        rtt_average(0),
        rtt_min(0),
        rtt_max(0),
        jitter(0)
    {
        pthread_mutex_init(&lock, NULL);
        pthread_cond_init(&cond, NULL);
    }

    ~ProbedPingTask()
    {
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&lock);
    }
};

#endif
