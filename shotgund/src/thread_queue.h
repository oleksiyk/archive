#ifndef SHOTGUND_THREAD_QUEUE_H
#define SHOTGUND_THREAD_QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "thread_manager.h"

template<typename C> class ThreadQueue
{
private:
    C * _C_container;
    size_t curSize;
    size_t maxSize;
    size_t waiting;

    size_t highWaterMark;

    ThreadManagerBase * threadManager;

    pthread_mutex_t containerLock;
    pthread_cond_t containerCond;

public:
    ThreadQueue(ThreadManagerBase * _threadManager,
        size_t _highWaterMark,
        size_t _maxSize = 16);

    ~ThreadQueue();

    int initialize();

    void push(const C c);
    int pop(C &c);
    int pop_nonblock(C &c);

    size_t getHighWaterMark()const{ return highWaterMark;}
    void setHighWaterMark(size_t _n){ highWaterMark = _n; }

    void lock() { pthread_mutex_lock(&containerLock); };
    void unlock() { pthread_mutex_unlock(&containerLock); };
};

// ----------------------

template<typename C>
ThreadQueue<C>::ThreadQueue(ThreadManagerBase * _threadManager,
        size_t _highWaterMark,
        size_t _maxSize):
    _C_container(NULL), curSize(0), maxSize(_maxSize),
    highWaterMark(_highWaterMark),
    threadManager(_threadManager)
{
}

template<typename C>
ThreadQueue<C>::~ThreadQueue()
{
    pthread_cond_destroy(&containerCond);
    pthread_mutex_destroy(&containerLock);

    free(_C_container);
}

template<typename C>
int ThreadQueue<C>::initialize()
{
    waiting = 0;

    pthread_mutex_init(&containerLock, NULL);

    if(pthread_cond_init(&containerCond, NULL) != 0){
        return -1;
    }

    _C_container = (C*)malloc(maxSize*sizeof(C));
    if(_C_container == NULL){
        return -1;
    }

    return 0;
}

template<typename C>
void ThreadQueue<C>::push(const C c)
{
    pthread_mutex_lock(&containerLock);

    if( curSize == maxSize ){
        maxSize *= 2;
        _C_container = (C*)realloc(_C_container, maxSize*sizeof(C));
        if(_C_container == NULL){
            pthread_mutex_unlock(&containerLock);
            return;
        }
    }

    if(curSize > highWaterMark){
        threadManager->onHighWatermark();
    }

    _C_container[curSize++] = c;

    pthread_cond_signal(&containerCond);
    pthread_mutex_unlock(&containerLock);
}

template<typename C>
int ThreadQueue<C>::pop(C &c)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    pthread_mutex_lock(&containerLock);

    while(curSize == 0){

        waiting++;
        pthread_cond_wait(&containerCond, &containerLock);
        waiting--;
    }

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    c = _C_container[--curSize];

    pthread_mutex_unlock(&containerLock);

    return 0;
}

template<typename C>
int ThreadQueue<C>::pop_nonblock(C &c)
{
    pthread_mutex_lock(&containerLock);

    if(curSize == 0 || waiting){
        pthread_mutex_unlock(&containerLock);
        return -1;
    }
    c = _C_container[--curSize];

    pthread_mutex_unlock(&containerLock);

    return 0;
}


#endif
