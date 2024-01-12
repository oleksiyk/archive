#ifndef SHOTGUND_THREAD_MANAGER_H
#define SHOTGUND_THREAD_MANAGER_H

#include <pthread.h>
#include "log.h"
#include <list>

class ThreadManagerBase
{
public:
    virtual ~ThreadManagerBase(){};

    virtual void onHighWatermark() = 0;
};

template<class queue_type, class thread_type> class ThreadManager : public ThreadManagerBase
{
private:
    queue_type * queue;
    //const Log * log;
    size_t threadsStart;
    size_t maxThreads;

    pthread_mutex_t threadsLock;

private:
    struct ThreadInfo
    {
        thread_type * thread;
        pthread_t thread_id;
    };
    std::list<ThreadInfo> threads;

public:
    ThreadManager(size_t _threadsStart, size_t _maxThreads);

    virtual ~ThreadManager();

    void onHighWatermark();

    int initialize(queue_type * _queue);

    int startThreads();

    int stopThreads();

    void removeTransientThread(pthread_t thr_id);
};

// ---------------------

template<class queue_type, class thread_type>
ThreadManager<queue_type, thread_type>::ThreadManager(size_t _threadsStart, size_t _maxThreads):
    queue(NULL),
    threadsStart(_threadsStart),
    maxThreads(_maxThreads)
{
    //log = __server->getLog();

    pthread_mutex_init(&threadsLock, NULL);
}

template<class queue_type, class thread_type>
ThreadManager<queue_type, thread_type>::~ThreadManager()
{
    typename std::list<ThreadInfo>::iterator i = threads.begin();

    pthread_mutex_lock(&threadsLock);

    for(; i != threads.end(); i++){
        delete (*i).thread;
    }

    pthread_mutex_unlock(&threadsLock);

    pthread_mutex_destroy(&threadsLock);
}

template<class queue_type, class thread_type>
void ThreadManager<queue_type, thread_type>::onHighWatermark()
{
    if(threads.size() >= maxThreads){
        //log->log(1, 0,0, "Server run out of maximum threads (%d)", maxThreads);
        return;
    }

    thread_type * thr = new thread_type(queue, true); //transient thread

    pthread_t thr_id = 0;

    if(thr->start(&thr_id) == -1){
        delete thr;
        return;
    }

    ThreadInfo ti = { thr, thr_id };
    threads.push_back(ti);

    queue->setHighWaterMark(queue->getHighWaterMark() + 16);
}

template<class queue_type, class thread_type>
int ThreadManager<queue_type, thread_type>::initialize(queue_type * _queue)
{
    queue = _queue;
    return 0;
}

template<class queue_type, class thread_type>
int ThreadManager<queue_type, thread_type>::startThreads()
{
    for(size_t i = 0; i<threadsStart; i++){
        thread_type * thr = new thread_type(queue, false);

        pthread_t thr_id = 0;

        if(thr->start(&thr_id) == -1){
            delete thr;
            stopThreads();
            return -1;
        }

        ThreadInfo ti = { thr, thr_id };
        threads.push_back(ti);
    }

    return 0;
}

template<class queue_type, class thread_type>
int ThreadManager<queue_type, thread_type>::stopThreads()
{
    pthread_mutex_lock(&threadsLock);

    typename std::list<ThreadInfo>::iterator i = threads.begin();
    for(; i != threads.end(); i++){
        ((*i).thread)->stop();
    }

    pthread_mutex_unlock(&threadsLock);

    return 0;
}

template<class queue_type, class thread_type>
void ThreadManager<queue_type, thread_type>::removeTransientThread(pthread_t thr_id)
{
    pthread_mutex_lock(&threadsLock);

    typename std::list<ThreadInfo>::iterator i = threads.begin();

    for(; i != threads.end(); i++){
        if((*i).thread_id == thr_id){
            threads.erase(i);
            break;
        }
    }

    pthread_mutex_unlock(&threadsLock);

    queue->lock();
    queue->setHighWaterMark(queue->getHighWaterMark() - 16);
    queue->unlock();
}

#endif
