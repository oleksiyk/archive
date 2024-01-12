/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#include "dac_thread.h"

DacThread::DacThread(int _fd)
{
    log = __server->getLog();

    fdset.fd = _fd;
    fdset.events = 0;
    fdset.revents = 0;

    log->log(9, "DacThread::DacThread()");
}

DacThread::~DacThread()
{
    close(fdset.fd);
    log->log(9, "DacThread::~DacThread()");
}

int DacThread::initialize()
{
    /*
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
    */

    return 0;
}

void * DacThread::threadFunc(void * p)
{
    DacThread * self = (DacThread*)p;

    dac_pthread_block_signal(SIGINT);
    dac_pthread_block_signal(SIGTERM);
    dac_pthread_block_signal(SIGHUP);
    dac_pthread_block_signal(SIGIO);

    dac_signal(SIGPIPE, SIG_IGN);

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    self->process();

    return NULL;
}

int DacThread::start()
{
    log->log(9, "thread::start");

    if(pthread_create(&threadSelf, NULL, threadFunc, this) != 0){
        log->log(1, "pthread_create() failed");
        return -1;
    }

    return 0;
}

void DacThread::stop()
{
    log->log(9, "thread::stop");

    shutdown = true;
    pthread_cancel(threadSelf);
    pthread_join(threadSelf, NULL);
}

void DacThread::process()
{
    char buf[256];

    fdset.events = POLLIN;
    fdset.revents = 0;

    for(;;){
        int i = poll(&fdset, 1, 1000);

        if(i == -1){
            log->log(1, "poll failed: %s", strerror(errno));
            break;
        }

        if(i > 0){
            if(fdset.revents & POLLIN){
                int k = read(fdset.fd, buf, 256);
                if(k > 0){
                    buf[k] = 0;
                    log->log(1, "buf='%s'", buf);
                } else if(k == -1) {
                    log->log(1, "read failed: %s", strerror(errno));
                    break;
                } else if(k == 0){
                    log->log(1, "connection closed from dac.nic.uk");
                    break;
                }
            } else if(fdset.revents & POLLERR){
                int sock_err = -1; socklen_t sock_err_len = sizeof(sock_err);
                if(getsockopt(fdset.fd, SOL_SOCKET, SO_ERROR, &sock_err, &sock_err_len) < 0){
                    log->log(1, "Connection failed: getsockopt failed: %s", strerror(errno));
                } else if(sock_err != 0){
                    log->log(1, "Connection failed: %s", strerror(sock_err));
                }
                break;
            } else if(fdset.revents & POLLHUP){
                log->log(1, "POLLHUP");
                break;
            }
        }

        if(i == 0){
            log->log(1, "poll() timeout");
        }

    }
}
