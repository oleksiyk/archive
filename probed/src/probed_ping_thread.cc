/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#include "probed_ping_thread.h"

ProbedPingThread::ProbedPingThread(ThreadQueue<ProbedPingTask*> * _queue, int _pingfd):
    queue(_queue), pingfd(_pingfd)
{
    log = __server->getLog();
}

ProbedPingThread::~ProbedPingThread()
{
    close(tcpfd);
}

void ProbedPingThread::threadCleanupFunc(void * p)
{
    ProbedPingThread * self = (ProbedPingThread*)p;

    self->queue->unlock();
}

void * ProbedPingThread::threadFunc(void * p)
{
    ProbedPingThread * self = (ProbedPingThread*)p;

    probed_pthread_block_signal(SIGINT);
    probed_pthread_block_signal(SIGTERM);
    probed_pthread_block_signal(SIGHUP);
    probed_pthread_block_signal(SIGIO);

    probed_signal(SIGPIPE, SIG_IGN);

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_cleanup_push(threadCleanupFunc, p);

    ProbedPingTask * obj = NULL;

    for(;;){
        self->queue->pop(obj);

        if(!_cmdTCP){
            if(self->sendICMPPing(obj) != -1){
                self->recvICMPPing(obj);
            }
        } else {
            if(self->sendTCPPing(obj) != -1){
                self->recvTCPPing(obj);
            }
            close(self->tcpfd); self->tcpfd = -1;
        }

        self->calculateResults(obj);

        pthread_mutex_unlock(&obj->lock);
        pthread_cond_signal(&obj->cond);
        pthread_mutex_unlock(&obj->lock);
    }

    pthread_cleanup_pop(0);

    return NULL;
}

int ProbedPingThread::start()
{
    log->log(9, "ping thread::start");

    if(pthread_create(&threadSelf, NULL, threadFunc, this) != 0){
        log->log(1, "pthread_create() failed");
        return -1;
    }

    return 0;
}

void ProbedPingThread::stop()
{
    log->log(9,  "ping thread::stop");

    //shutdown = true;
    pthread_cancel(threadSelf);
    pthread_join(threadSelf, NULL);
}

int ProbedPingThread::sendICMPPing(ProbedPingTask * obj)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int len = 56 +8;
    struct icmp *icmp;

    int flags = fcntl(pingfd, F_GETFL);
    flags = flags & ~O_NONBLOCK;
    if(fcntl(pingfd, F_SETFL, flags) == -1){
        log->log(1, "[%s] setting ~O_NONBLOCK failed: %s", obj->sslPeerName, strerror(errno));
        return -1;
    }

    icmp = (struct icmp*)malloc(len);
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = getpid();

    for(int i=0; i<obj->packets; i++){

        icmp->icmp_seq = i;
        gettimeofday((struct timeval*)icmp->icmp_data, NULL);

        icmp->icmp_cksum = 0;
        icmp->icmp_cksum = in_cksum((u_short*)icmp, len);

        if(sendto(pingfd, icmp, len, 0, (const struct sockaddr*)&obj->addr, addrlen) == -1){
            log->log(1, "[%s] ICMP ECHO sendto() failed: %s", obj->sslPeerName, strerror(errno));
            return -1;
        }

        //probed_sleep(0, 10);

    }

    return 0;
}

int ProbedPingThread::recvICMPPing(ProbedPingTask * obj)
{
    char recvbuf[ICMP_BUFSIZE];
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    struct timeval tvrecv, tvlast;

    char name[INET_ADDRSTRLEN];

    struct pollfd fds;

    int flags = fcntl(pingfd, F_GETFL);
    flags = flags | O_NONBLOCK;
    if(fcntl(pingfd, F_SETFL, flags) == -1){
        log->log(1, "[%s] setting O_NONBLOCK failed: %s", obj->sslPeerName, strerror(errno));
        return -1;
    }

    fds.fd = pingfd;
    fds.events = POLLIN;
    fds.revents = 0;

    int i =0;
    int k = 0;
    gettimeofday(&tvlast, NULL);
    while(i < obj->packets){
        k = poll(&fds, 1, 1000);
        if(k == -1){
            log->log(0, "[%s] ICMP poll() failed: %s", obj->sslPeerName, strerror(errno));
            break;
        }

        if(k > 0){
            if(fds.revents & POLLIN){
                int z = 0;
                while( (z = recvfrom(pingfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&addr, &addrlen)) > 0 && i < obj->packets ){
                    gettimeofday(&tvrecv, NULL);
                    inet_ntop(AF_INET, &addr.sin_addr, name, INET_ADDRSTRLEN);
                    name[INET_ADDRSTRLEN-1] = 0;
                    if(processICMPPacket(recvbuf, obj, &tvrecv, z, name)){
                        float ms = (tvrecv.tv_sec - tvlast.tv_sec)*1000.0 + float((tvrecv.tv_usec - tvlast.tv_usec))/1000.0;
                        if(ms > 1000){
                            k = 0;
                            break;
                        }
                        continue;
                    }
                    memcpy(&tvlast, &tvrecv, sizeof(struct timeval));
                    i++;
                }
                if( z < 0 && errno != EAGAIN){
                    log->log(1, "[%s] ICMP recvfrom failed: %s", obj->sslPeerName, strerror(errno));
                    break;
                }
            }
        }
        if( k == 0){
            log->log(1, "[%s] ICMP timeout", obj->sslPeerName);
            break;
        }
    }

    return 0;
}

int ProbedPingThread::processICMPPacket(char * ptr, ProbedPingTask * obj, struct timeval * tvrecv, socklen_t len, const char * name)
{
    int hlen1, icmplen;
    struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;

    ip = (struct ip*)ptr;
    hlen1 = ip->ip_hl << 2;

    icmp = (struct icmp*)(ptr + hlen1);
    if((icmplen = len - hlen1) < 8){
        log->log(1, "[%s] broken ICMP packet received from %s: icmplen (%d) < 8", obj->sslPeerName, name, icmplen);
        return -1;
    }

    if(icmp->icmp_type == ICMP_ECHOREPLY){
        if(icmp->icmp_id != getpid()){
            return -1;
        }
        if(icmplen < 16){
            log->log(1, "[%s] broken ICMP packet received from %s: icmplen (%d) < 16", obj->sslPeerName, name, icmplen);
            return -1;
        }

        tvsend = (struct timeval*)icmp->icmp_data;
        float rtt = (tvrecv->tv_sec - tvsend->tv_sec)*1000.0 + float((tvrecv->tv_usec - tvsend->tv_usec))/1000.0;

        log->log(4, "[%s] %d bytes from %s: seq=%u, ttl=%d, rtt=%.3fms", obj->sslPeerName,
            icmplen, name, icmp->icmp_seq, ip->ip_ttl, rtt);
        obj->rtts.push_back(rtt);

    } else {
        log->log(4, "[%s] %d bytes from %s: type=%d, code=%d", obj->sslPeerName,
            icmplen, name, icmp->icmp_type, icmp->icmp_code);
    }

    return 0;
}

int ProbedPingThread::sendTCPPing(ProbedPingTask * obj)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    char name[INET_ADDRSTRLEN];
    struct pollfd fds;
    char echo_string[128];
    strncpy(echo_string, __server->getConfig()->findNode("echo_string")->getValue(), 126);
    strncat(echo_string, "\n", 2);

    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    if(tcpfd == -1){
        log->log(1, "[%s] socket() failed: %s", obj->sslPeerName, strerror(errno));
        return -1;
    }

    if(fcntl(tcpfd, F_SETFL, O_NONBLOCK) == -1){
        log->log(1, "[%s] setting O_NONBLOCK failed: %s", obj->sslPeerName, strerror(errno));
        return -1;
    }

    obj->addr.sin_port = htons(7);

    fds.fd = tcpfd;
    fds.events = POLLOUT;
    fds.revents = 0;

    inet_ntop(AF_INET, &obj->addr.sin_addr, name, INET_ADDRSTRLEN);
    name[INET_ADDRSTRLEN-1] = 0;

    if(connect(tcpfd, (const struct sockaddr *)&obj->addr, addrlen) == -1 ){
        if(errno == EINPROGRESS){
            int k = poll(&fds, 1, 2500);
            if(k == -1){
                log->log(0, "[%s] TCP poll() failed: %s", obj->sslPeerName, strerror(errno));
                return -1;
            }
            if(k > 0){
                if(fds.revents & POLLOUT){
                    socklen_t s_len = sizeof(int);
                    int code;
                    if(getsockopt(tcpfd, SOL_SOCKET, SO_ERROR, &code, &s_len) == -1){
                        log->log(1, "[%s] getsockopt() failed: %s", obj->sslPeerName, strerror(errno));
                        return -1;
                    }
                    if(code != 0){
                        log->log(1, "[%s] connect(2) failed: %s", obj->sslPeerName, strerror(code));
                        return -1;
                    }
                }
            }
            if( k == 0){
                log->log(1, "[%s] connection to %s timed out", obj->sslPeerName, name);
                return -1;
            }
        } else {
            log->log(1, "[%s] connect(1) failed: %s", obj->sslPeerName, strerror(errno));
            return -1;
        }
    }

    log->log(4, "[%s] connected to %s", obj->sslPeerName, name);

    gettimeofday(&tcptv[0], NULL);

    int i = 0;
    int k = 0;
    while(i < obj->packets){
        k = poll(&fds, 1, 1000);
        if(k == -1){
            log->log(0, "[%s] TCP poll() failed: %s", obj->sslPeerName, strerror(errno));
            break;
        }

        if(k > 0){
            if(fds.revents & POLLOUT){
                int z = 0;
                while((z = send(tcpfd, echo_string, strlen(echo_string), 0)) > 0 && i < obj->packets){
                    i++;
                }
                if( z < 0 && errno != EAGAIN){
                    log->log(1, "[%s] TCP send() failed: %s", obj->sslPeerName, strerror(errno));
                    break;
                }
            }
        }
        if( k == 0){
            log->log(1, "[%s] TCP send timeout", obj->sslPeerName);
            break;
        }
    }

    return 0;
}

int ProbedPingThread::recvTCPPing(ProbedPingTask * obj)
{
    struct pollfd fds;
    char buf[128];
    int echo_len = strlen(__server->getConfig()->findNode("echo_string")->getValue()) + 1;
    char name[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &obj->addr.sin_addr, name, INET_ADDRSTRLEN);
    name[INET_ADDRSTRLEN-1] = 0;

    fds.fd = tcpfd;
    fds.events = POLLIN;
    fds.revents = 0;

    int i = 0;
    int k = 0;
    while(i < obj->packets){
        k = poll(&fds, 1, 1000);
        if(k == -1){
            log->log(0, "[%s] TCP poll() failed: %s", obj->sslPeerName, strerror(errno));
            break;
        }

        if(k > 0){
            if(fds.revents & POLLIN){
                int z = 0;
                while((z = recv(tcpfd, buf, echo_len, 0)) > 0 && i < obj->packets){
                    buf[z] = 0;
                    gettimeofday(&tcptv[1], NULL);

                    float rtt = (tcptv[1].tv_sec - tcptv[0].tv_sec)*1000.0 + float((tcptv[1].tv_usec - tcptv[0].tv_usec))/1000.0;

                    log->log(4, "[%s] %d bytes from %s: seq=%d, rtt=%.3fms", obj->sslPeerName,
                        z, name, i, rtt);
                    obj->rtts.push_back(rtt);
                    i++;
                }
                if( z < 0 && errno != EAGAIN){
                    log->log(1, "[%s] TCP recv() failed: %s", obj->sslPeerName, strerror(errno));
                    break;
                }
            }
        }
        if( k == 0){
            log->log(1, "[%s] TCP recv timeout", obj->sslPeerName);
            break;
        }
    }

    return 0;
}


int ProbedPingThread::calculateResults(ProbedPingTask * obj)
{
    int k = 0;
    std::list<float>::iterator i = obj->rtts.begin();

    float rtt_average = 0;
    float jitter = 0;

    for(; i != obj->rtts.end(); i++){
        rtt_average += (*i);
        k++;
    }

    rtt_average = rtt_average / k;

    for(i = obj->rtts.begin(); i != obj->rtts.end(); i++){
        float dev = fabsf((*i) - rtt_average);
        jitter += dev*dev;
    }

    jitter = sqrtf(jitter/k);

    std::list<float>::const_iterator rtt_min = std::min_element(obj->rtts.begin(), obj->rtts.end());
    std::list<float>::const_iterator rtt_max = std::max_element(obj->rtts.begin(), obj->rtts.end());

    obj->packets_recvd = k;
    obj->rtt_average = rtt_average;
    obj->rtt_min = (*rtt_min);
    obj->rtt_max = (*rtt_max);
    obj->jitter = jitter;

    return 0;

}


