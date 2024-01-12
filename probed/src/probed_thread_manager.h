/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_THREAD_MANAGER_H
#define PROBED_THREAD_MANAGER_H

#include "decl.h"
#include <list>

class ProbedThreadManager
{
private:
    const Log * log;
    pthread_mutex_t threadsLock;

private:
    std::list<ProbedThread*> threads;

public:
    ProbedThreadManager();

    virtual ~ProbedThreadManager();

    int startThread(int fd);

    void removeThread(ProbedThread * thread);

    int stopThreads();
};

#endif
