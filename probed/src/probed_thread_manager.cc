/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#include "probed_thread_manager.h"

ProbedThreadManager::ProbedThreadManager()
{
    log = __server->getLog();

    pthread_mutex_init(&threadsLock, NULL);
}

ProbedThreadManager::~ProbedThreadManager()
{
    std::list<ProbedThread*>::iterator i = threads.begin();

    pthread_mutex_lock(&threadsLock);

    for(; i != threads.end(); i++){
        delete (*i);
    }

    pthread_mutex_unlock(&threadsLock);

    pthread_mutex_destroy(&threadsLock);
}

int ProbedThreadManager::startThread(int fd)
{
    ProbedThread * thr = new ProbedThread(fd);

    if(thr->start() == -1){
        delete thr;
        stopThreads();
        return -1;
    }

    threads.push_back(thr);

    return 0;
}

int ProbedThreadManager::stopThreads()
{
    pthread_mutex_lock(&threadsLock);

    std::list<ProbedThread*>::iterator i = threads.begin();

    for(; i != threads.end(); i++){
        (*i)->stop();
    }

    pthread_mutex_unlock(&threadsLock);

    return 0;
}

void ProbedThreadManager::removeThread(ProbedThread * thread)
{
    pthread_mutex_lock(&threadsLock);

    std::list<ProbedThread*>::iterator i = threads.begin();

    for(; i != threads.end(); i++){
        if((*i) == thread){
            threads.erase(i);
            break;
        }
    }

    pthread_mutex_unlock(&threadsLock);
}
