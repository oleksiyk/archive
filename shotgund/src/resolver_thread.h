#ifndef SHOTGUND_THREAD_H
#define SHOTGUND_THREAD_H

#include "decl.h"
#include <math.h>

class ResolverThread
{
private:
    ThreadQueue<QueueObj*> * queue;
    const Log * log;
    ShotgundResolver * resolvers[MAX_PAR];
    struct pollfd fds[MAX_PAR];
    struct timeval tv[2];

    bool shutdown;
    bool transient; // temporary thread

    pthread_t threadSelf;

    static void * threadFunc(void * p);
    static void threadCleanupFunc(void * p);

private:
    const char * checkSuffix(const char * name, const char * suffix);
    char * iRewrite(char * qname);
    char * jRewrite(char * qname);

private:
    int dorequest_timeoutson(QueueObj * obj, int i);
    int dorequest_timeoutsoff(QueueObj * obj, int i);
    int dorequest(QueueObj * obj, int i);
    int checkreply(QueueObj * obj, int k);
    int doreply(QueueObj * obj, int k);
    const char * getQTypeStr(u_int8_t qtype);
    void dump(ShotgundResolver * resolver, QueueObj * obj, const char * prefix);
    int replyNXDOMAIN(QueueObj * obj);

    //int sendICMPUnreach(const sockaddr_in * dest);

public:
    ResolverThread(ThreadQueue<QueueObj*> * _queue, bool _transient);
    virtual ~ResolverThread();

    int start(pthread_t * thr_id);
    void stop();
};

#endif
