/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#ifndef DAC_THREAD_H
#define DAC_THREAD_H

#include "decl.h"
#include <memory>

class DacThread
{
private:
    const Log * log;
    struct pollfd fdset;

    bool shutdown;

    pthread_t threadSelf;

    int initialize();
    static void * threadFunc(void * p);
    void process();

public:
    DacThread(int _fd);
    virtual ~DacThread();

    int start();
    void stop();
};

#endif
