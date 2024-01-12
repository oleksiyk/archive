/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#include "probed_thread.h"

ProbedThread::ProbedThread(int _fd)
{
    log = __server->getLog();

    fdset.fd = _fd;
    fdset.events = 0;
    fdset.revents = 0;

    log->log(9, "ProbedThread::ProbedThread()");

    tr = NULL;
}

ProbedThread::~ProbedThread()
{
    if(tr){
        tr->shutdown(true);
        delete tr;
    }
    close(fdset.fd);
    log->log(9, "ProbedThread::~ProbedThread()");
}

int ProbedThread::initialize()
{
    if(fcntl(fdset.fd, F_SETFL, O_NONBLOCK) == -1){
        log->log(1, "setting O_NONBLOCK failed: %s", strerror(errno));
        close(fdset.fd);
        return -1;
    }

    struct sockaddr_in peer;
    peerName[0] = 0;
    socklen_t namelen = sizeof(struct sockaddr_in);

    if (getpeername(fdset.fd, (struct sockaddr *)&peer, &namelen) == -1) {
        log->log(1, "getpeername() failed: %s", strerror(errno));
        return -1;
    }

    inet_ntop(AF_INET, &peer.sin_addr, peerName, INET_ADDRSTRLEN);
    peerName[INET_ADDRSTRLEN-1] = 0;
    snprintf(&peerName[strlen(peerName)], 7, ":%d", ntohs(peer.sin_port));
    peerName[INET_ADDRSTRLEN + 6] = 0;

    log->log(1, "[%s] Connection request", peerName);

    return 0;
}

int ProbedThread::process()
{
    int timeout = DEFAULT_SESSION_TIMEOUT;

    const DOTCONFDocumentNode * node = __server->getConfig()->findNode("session_timeout");
    if(node){
        timeout = atoi(node->getValue());
    }

    tr = new ProbedSSLTransport(this, timeout);

    if(tr->initialize() == -1){
        return -1;
    }

    if(processCommands() == -1){
        return -1;
    }

    tr->shutdown();

    delete tr; tr = NULL;

    //close(fdset.fd);
    //log->log(1, "[%s] Connection closed", peerName);

    return 0;
}

void * ProbedThread::threadFunc(void * p)
{
    ProbedThread * self = (ProbedThread*)p;

    probed_pthread_block_signal(SIGINT);
    probed_pthread_block_signal(SIGTERM);
    probed_pthread_block_signal(SIGHUP);
    probed_pthread_block_signal(SIGIO);

    probed_signal(SIGPIPE, SIG_IGN);

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    if(self->initialize() != -1){
        self->process();
    }

    __server->getThreadManager()->removeThread(self);
    delete self;

    return NULL;
}

int ProbedThread::start()
{
    log->log(9, "thread::start");

    if(pthread_create(&threadSelf, NULL, threadFunc, this) != 0){
        log->log(1, "pthread_create() failed");
        return -1;
    }

    return 0;
}

void ProbedThread::stop()
{
    log->log(9, "thread::stop");

    shutdown = true;
    pthread_cancel(threadSelf);
    pthread_join(threadSelf, NULL);
}

int ProbedThread::processCommands()
{
    char buf[256];
    std::auto_ptr<ProbedCommands> prc(new ProbedCommands(log, tr, this));
    int i = 0;

    while(!i){
        buf[0] = 0;

        int k = tr->read(buf, 256);

        if(k == 0){ //connection closed
            tr->shutdown(true);
            return 0;
        }
        if(k == -1){ //error
            tr->shutdown(true);
            return -1;
        }

        if(k >= 256){
            log->log(1, "[%s] line too long", peerName);
            tr->shutdown(true);
            return -1;
        }
        buf[k] = 0;

        char * cmd = probed_strchop(buf);
        if(strlen(cmd)){
            probed_strtoupper(cmd);
            i = prc->process(cmd);
        }
    }

    return 0;
}

