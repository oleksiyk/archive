#include "resolver_thread.h"

ResolverThread::ResolverThread(ThreadQueue<QueueObj*> * _queue, bool _transient):
    queue(_queue),
    shutdown(false), transient(_transient)
{
    log = __server->getLog();

    //log->log(4,0,0,"ResolverThread::ResolverThread()");

    for(size_t i=0;i<MAX_PAR;i++){
        resolvers[i] = NULL;
    }

    resolvers[0] = new ShotgundResolver(log);

}

ResolverThread::~ResolverThread()
{
    for(size_t i=0;i<MAX_PAR;i++){
        delete resolvers[i];
    }

    //log->log(4,0,0,"ResolverThread::~ResolverThread()");
}

void ResolverThread::threadCleanupFunc(void * p)
{
    ResolverThread * self = (ResolverThread*)p;

    self->queue->unlock();
}

void * ResolverThread::threadFunc(void * p)
{
    ResolverThread * self = (ResolverThread*)p;

    shotgund_pthread_block_signal(SIGINT);
    shotgund_pthread_block_signal(SIGTERM);
    shotgund_pthread_block_signal(SIGHUP);
    shotgund_pthread_block_signal(SIGIO);

    shotgund_signal(SIGPIPE, SIG_IGN);

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    pthread_cleanup_push(threadCleanupFunc, p);

    QueueObj* obj = NULL;
    char * qname = NULL;
    char * hzone = NULL;
    u_int16_t   qtype;
    u_int16_t   qclass;
    u_int16_t orig_qid = 0;
    u_int8_t recursive = 0;
    const RFC1035_MessageQuestion * q = NULL;
    bool permit = false;
    const DOTCONFDocumentNode * node = NULL;

    for(;;){
        if(self->shutdown)
            break;

        AsyncDNSReply * reply = NULL;

        self->queue->pop(obj);

        qname = NULL;
        reply = self->resolvers[0]->getReply();

        if(reply->addparse(&obj->msg, obj->msgLen) == -1){
            self->log->log(4, obj->id, 0, "reply->addparse failed");
            goto cleanup;
        }
        orig_qid = reply->getHeader()->id;
        recursive = reply->getHeader()->get_rd();
        q = reply->getQuestion();
        qtype = q->qtype;
        qclass = q->qclass;
        qname = strdup(q->qname);
        hzone = NULL;

        self->log->log(4, obj->id, 0, "New lookup from host %s \"%s %s\"", obj->addr, self->getQTypeStr(qtype), qname);

        permit = false;  node = NULL;
        while((node = __server->getConfig()->findNode("host", NULL, node)) != NULL){
            const char * s = NULL;
            if(!fnmatch(node->getValue(0), obj->addr, 0)){
                //check RR type
                if(!strchr(node->getValue(2), '*')){
                    if(strcasecmp(self->getQTypeStr(qtype), node->getValue(2))) continue;
                }
                if(strchr(node->getValue(1), '*')){ //is it wildcard?
                    if(!fnmatch(node->getValue(1), qname, 0)){
                        break;
                    }
                } else if((s = self->checkSuffix(qname, node->getValue(1))) != NULL){
                    hzone = strdup(s);
                    break;
                }
            }
        }
#ifdef CRIPPLED
        if(qtype != RFC1035_TYPE_NAPTR){
            hzone = NULL; //this will disable zone suffix rewrites for non NAPTR lookups
        }
#endif
        if(node){
            if(!strcasecmp(node->getValue(3), "permit")) permit = true;
            self->log->log(4, obj->id, 0, "Lookup matched ACL \"host %s %s %s %s\" - %s", node->getValue(0),
                node->getValue(1),
                self->getQTypeStr(qtype),
                node->getValue(3),
                permit?"permitted":"denied");
        } else {
            self->log->log(4, obj->id, 0, "Lookup does not matched any ACL - denied");
            //permit = true;
        }

        if(permit){

            bool reply_ok = false;

            const char * s = NULL;
            //int grid = 0;
            int i = 0; int i2= 0;
            int weight = 1;
            bool norewrite = false; // 'n' option in host line
            bool ioption = false; // 'i' option in thost line
            bool joption = false; // 'j' option in host line

            // some options
            int g = 4;
            while((s = node->getValue(g++)) != NULL){
                if(s[0] == 'n' && s[1] == 0){
                    self->log->log(4, obj->id, 0, "Response rewriting disabled");
                    norewrite = true;
                } else if(s[0] == 'i' && s[1] == 0){
                    ioption = true;
                } else if(s[0] == 'j' && s[1] == 0){
                    joption = true;
                }
            }
            if(strchr(qname, '*') && qtype == RFC1035_TYPE_NAPTR){ // if query contains '*' AND it is NAPTR then check i/j options
                if(ioption){
                    qname = self->iRewrite(qname);
                    self->log->log(4, obj->id, 0, "Rewriting query to %s ('i' option)", qname);
                } else if(joption){
                    qname = self->jRewrite(qname);
                    self->log->log(4, obj->id, 0, "Rewriting query to %s ('j' option)", qname);
                }
            }

            // move through the groups
            g = 4;
            while((s = node->getValue(g++)) != NULL){
                //if(!isdigit(s[0])) break;
                if(s[1] == 0 && (s[0] == 'i' || s[0] == 'j' || s[0] == 'n')) break; //this is options (i,j,n), there wouldn't be groups nay more
                //grid = atoi(s);
                self->log->log(4, obj->id, 0, "Group '%s' start", s);

                const DOTCONFDocumentNode * fnode = NULL;
                const DOTCONFDocumentNode * znode = NULL;
                i = 0;
                while((znode = __server->getConfig()->findNode("zone", NULL, znode)) != NULL){
                    //if(atoi(znode->getValue()) != atoi(s)) continue;
                    if(strcmp(znode->getValue(), s)) continue;

                    weight = atoi(znode->getValue(2));

                    char * qsname = (char*)malloc(strlen(qname) + strlen(znode->getValue(1)));
                    strcpy(qsname, qname);
                    if(hzone != NULL) { //rewrite zone suffix
                        strcpy((char*)self->checkSuffix(qsname, hzone), znode->getValue(1));
                        //remove trailing dot if any
                        if(qsname[strlen(qsname)-1] == '.') qsname[strlen(qsname)-1] = 0;
                    }

                    while((fnode = __server->getConfig()->findNode("forwarder", NULL, fnode)) != NULL){

                        //if(atoi(fnode->getValue()) != atoi(s)) continue;
                        if(strcmp(fnode->getValue(), znode->getValue(4))) continue;

                        i++; i2++;
                        if(i>MAX_PAR){
                            self->log->log(1, obj->id, 0, "Too much parallel lookups defined for group %s, limiting  to %d lookups", s, MAX_PAR);
                            break;
                        }

                        if(self->resolvers[i-1] == NULL){
                            self->resolvers[i-1] = new ShotgundResolver(self->log);
                            if(self->resolvers[i-1]->initialize() == -1){
                                self->log->log(1, obj->id, i2, "failed to initialize resolver");
                                break;
                            }
                        }
                        int port = 53; const char * ps = fnode->getValue(2);
                        if(ps != NULL){
                            port = atoi(ps);
                        }

                        self->resolvers[i-1]->setNameserver(fnode->getValue(1), port);
                        self->resolvers[i-1]->setQueueObj(obj, i2);
                        self->resolvers[i-1]->prepareQuery(RFC1035_OPCODE_QUERY, recursive);
                        self->resolvers[i-1]->getQueryMessage()->header.id = orig_qid;
                        self->resolvers[i-1]->addQuery(qsname, qtype, qclass);
                        self->resolvers[i-1]->waitms = atoi(znode->getValue(3));
                        self->resolvers[i-1]->weight = weight;
                        self->resolvers[i-1]->norewrite = norewrite;
                        self->log->log(4, obj->id, i2, "Request: %s %s @%s, %sms, weight %d", self->getQTypeStr(qtype), qsname, fnode->getValue(1), znode->getValue(3), weight);
#ifdef CRIPPLED
                        break; //only one forwarder
#endif
                    }

                    free(qsname);

                    if(i>MAX_PAR) break;
#ifdef CRIPPLED
                    break; //only one zone
#endif
                }

                if( i == 0){ //no zone lines for this group
                    while((fnode = __server->getConfig()->findNode("forwarder", NULL, fnode)) != NULL){
                        //if(atoi(fnode->getValue()) != atoi(s)) continue;
                        if(strcmp(fnode->getValue(), s)) continue;

                        i++;
                        if(i>MAX_PAR){
                            self->log->log(4, obj->id, 0, "Too much parallel lookups defined for group %s, limiting  to %d lookups", s, MAX_PAR);
                            i--;
                            break;
                        }

                        if(self->resolvers[i-1] == NULL){
                            self->resolvers[i-1] = new ShotgundResolver(self->log);
                            if(self->resolvers[i-1]->initialize() == -1){
                                self->log->log(1, obj->id, i, "failed to initialize resolver");
                                break;
                            }
                        }
                        int port = 53; const char * ps = fnode->getValue(2);
                        if(ps != NULL){
                            port = atoi(ps);
                        }

                        self->resolvers[i-1]->setNameserver(fnode->getValue(1), port);
                        self->resolvers[i-1]->setQueueObj(obj, i);
                        self->resolvers[i-1]->prepareQuery(RFC1035_OPCODE_QUERY, recursive);
                        self->resolvers[i-1]->getQueryMessage()->header.id = orig_qid;
                        self->resolvers[i-1]->addQuery(qname, qtype, qclass);
#ifndef CRIPPLED
                        if(fnode->getValue(3) != NULL){
                            self->resolvers[i-1]->waitms = atoi(fnode->getValue(3));
                        }
#endif
                        self->log->log(4, obj->id, i, "Request: %s %s @%s, %dms, weight %d", self->getQTypeStr(qtype), qname, fnode->getValue(1), self->resolvers[i-1]->waitms, weight);
                    }
                }

                // resolve group by group
                if(self->dorequest(obj, i) > 0){
                    reply_ok = true;  break;
                }
            }

            if(!reply_ok){
                self->replyNXDOMAIN(obj);
            }

        } else { //denied
            //self->sendICMPUnreach(&obj->in);
        }

cleanup:
        for(int k=0;k<MAX_PAR;k++){
            if(self->resolvers[k])
                self->resolvers[k]->cleanup();
        }

        delete obj;
        free(qname); qname = NULL;
        free(hzone); hzone = NULL;

        if(self->shutdown)
            break;
    }

    pthread_cleanup_pop(0);

    if(self->transient && !self->shutdown){
        ((Shotgund::ThreadManager*)(__server)->getThreadManager())->removeTransientThread(self->threadSelf);
        delete self;
    }

    return NULL;
}

int ResolverThread::replyNXDOMAIN(QueueObj * obj)
{
    int k = 0;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    float ms = (tv[1].tv_sec - tv[0].tv_sec)*1000 + float((tv[1].tv_usec - tv[0].tv_usec))/1000;

    log->log(4, obj->id, 0, "Return: NXDOMAIN, %.2fms", ms);

    resolvers[0]->getQueryMessage()->header.set_rcode(RFC1035_RCODE_NXDOMAIN);
    resolvers[0]->getQueryMessage()->header.set_qr(1);

    if(!resolvers[0]->norewrite){

        struct iovec ivec[2];
        struct msghdr hdr;

        ivec[0].iov_base = &resolvers[0]->getQueryMessage()->header;
        ivec[0].iov_len = 12;
        ivec[1].iov_base = &obj->msg.buf;
        ivec[1].iov_len = obj->msgLen - 12;

        hdr.msg_name = &(obj->in);
        hdr.msg_namelen = addrlen;
        hdr.msg_iov = &ivec[0];
        hdr.msg_iovlen = 2;
        hdr.msg_control = NULL;
        hdr.msg_controllen = 0;
        hdr.msg_flags = 0;

        if((k = sendmsg(obj->fd, &hdr, 0)) == -1){
            log->log(1, obj->id, resolvers[0]->getSubid(), "sendto failed: %s", strerror(errno));
        }

    } else {

        if((k = sendto(obj->fd, resolvers[0]->getQueryMessage(), resolvers[0]->getQueryMessageSize(), 0, (struct sockaddr*)&(obj->in), addrlen)) == -1){
            log->log(1, obj->id, 0, "sendto failed: %s", strerror(errno));
        }
    }

    return k;
}

int ResolverThread::dorequest(QueueObj * obj, int i)
{
    if(__server->getConfig()->async_query){
        return dorequest_timeoutsoff(obj, i);
    }
    return dorequest_timeoutson(obj, i);
}

int ResolverThread::dorequest_timeoutsoff(QueueObj * obj, int i)
{
    int wait = 0;
    float elapsed = 0;
    bool dopoll = false;
    int minweight = 0;
    int replied = -1;

    gettimeofday(&tv[0], NULL);

    for(;;){
        minweight = 0;
        for(int k=0;k<i;k++){
            if(resolvers[k]->skip) continue;
            if(minweight > resolvers[k]->weight || minweight == 0){
                minweight = resolvers[k]->weight;
            }
        }
        //log->log(4,obj->id, 0, "minweight=%d", minweight);
        wait = 0; dopoll = false;
        for(int k=0;k<i;k++){
            if(resolvers[k]->skip) continue;
            if(resolvers[k]->resolveQueries() != EAGAIN){
                int r = checkreply(obj, k);
                if(r != 0){ //valid reply
                    if(resolvers[k]->weight <= minweight){
                        return doreply(obj, k);
                    } else {
                        if(replied == -1 || resolvers[k]->weight < resolvers[replied]->weight){
                            replied = k;
                        }
                        resolvers[k]->skip = true;
                        fds[k].fd = -1; continue;
                    }
                } else {
                    resolvers[k]->skip = true;
                    fds[k].fd = -1; continue;
                }
            }
            if(wait < resolvers[k]->waitms || wait == 0){
                wait = resolvers[k]->waitms;
            }
            memcpy(&fds[k], resolvers[k]->getPollfd(), sizeof(struct pollfd));
            dopoll = true;
        }
        int e = 0;
        if(dopoll){
            wait = wait - int(rintf(elapsed));
            //log->log(4,obj->id, 0, "async_on polling with wait=%d", wait);
            if(wait >= 1)
                e = poll(fds, i, wait);
        }
        gettimeofday(&tv[1], NULL);
        elapsed = (tv[1].tv_sec - tv[0].tv_sec)*1000 + float((tv[1].tv_usec - tv[0].tv_usec))/1000;
        if(e == -1){
            log->log(1, obj->id, 0, "poll() failed: %s", strerror(errno));
            return -1;
        }
        if(e == 0){
            for(int k=0;k<i;k++){
                if(resolvers[k]->skip) continue;
                log->log(1, obj->id, resolvers[k]->getSubid(), "Timeout: %.2fms", elapsed);
            }
            if(replied != -1){
                return doreply(obj, replied);
            }
            return 0;
        }
    }

    return 0;
}

int ResolverThread::dorequest_timeoutson(QueueObj * obj, int i)
{
    int wait = 0;
    float elapsed = 0;
    bool dopoll = false;
    int minweight = 0;
    int replied = -1;

    gettimeofday(&tv[0], NULL);

    for(;;){
        wait = 0; dopoll = false;
        minweight = 0;
        for(int k=0;k<i;k++){
            if(resolvers[k]->skip) continue;
            if(minweight > resolvers[k]->weight || minweight == 0){
                minweight = resolvers[k]->weight;
            }
        }
        for(int k=0;k<i;k++){
            if(resolvers[k]->skip) continue;
            if(resolvers[k]->resolveQueries() != EAGAIN){
                int r = checkreply(obj, k);
                if(r != 0){ //valid reply
                    if(resolvers[k]->weight <= minweight){
                        return doreply(obj, k);
                    } else {
                        if(replied == -1 || resolvers[k]->weight < resolvers[replied]->weight){
                            replied = k;
                        }
                        resolvers[k]->skip = true;
                        fds[k].fd = -1; continue;
                    }
                } else {
                    resolvers[k]->skip = true;
                    fds[k].fd = -1; continue;
                }
            }
            if(wait > resolvers[k]->waitms || wait == 0){
                wait = resolvers[k]->waitms;
            }
            memcpy(&fds[k], resolvers[k]->getPollfd(), sizeof(struct pollfd));
            dopoll = true;
        }
        int e = 0;
        if(dopoll){
            wait = wait - int(rintf(elapsed));
            //log->log(4,obj->id, 0, "async_off polling with wait=%d", wait);
            if(wait >= 1)
                e = poll(fds, i, wait);
        }
        gettimeofday(&tv[1], NULL);
        elapsed = (tv[1].tv_sec - tv[0].tv_sec)*1000 + float((tv[1].tv_usec - tv[0].tv_usec))/1000;
        if(e == -1){
            log->log(1, obj->id, 0, "poll() failed: %s", strerror(errno));
            return -1;
        }
        if(e >= 0){
            bool fulltimeout = true;
            for(int k=0;k<i;k++){
                if(resolvers[k]->skip) continue;
                if(resolvers[k]->waitms <= int(rintf(elapsed))){
                    log->log(4, obj->id, resolvers[k]->getSubid(), "Timeout: %.2fms", elapsed);
                    resolvers[k]->skip = true;
                    fds[k].fd = -1;
                } else {
                    fulltimeout = false;
                }
            }
            if(fulltimeout){ //full group timeout
                if(replied != -1)
                    return doreply(obj, replied);
                return 0;
            }
        }
    }

    return 0;
}

int ResolverThread::checkreply(QueueObj * obj, int k)
{
    if(resolvers[k]->getResponseMessageSize()){

        //gettimeofday(&tv[1], NULL);

        float elapsed = (tv[1].tv_sec - tv[0].tv_sec)*1000 + float((tv[1].tv_usec - tv[0].tv_usec))/1000;

        if(resolvers[k]->waitms <= int(rintf(elapsed))){
            dump(resolvers[k], obj, "Reply (Skipped): ");
            return 0;
        } else {
            dump(resolvers[k], obj, "Reply: ");
        }

        if(resolvers[k]->getReply()->getAnswer() == NULL){
            return 0;
        }
    }

    return resolvers[k]->getResponseMessageSize();
}


int ResolverThread::doreply(QueueObj * obj, int k)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if(resolvers[k]->getResponseMessageSize()){

        if(resolvers[k]->getReply()->getAnswer() == NULL){
            return 0;
        }

        dump(resolvers[k], obj, "Return: ");

        if(!resolvers[k]->norewrite){

            struct iovec ivec[3];
            struct msghdr hdr;

            resolvers[k]->getResponseMessage()->header.set_nscount(0);
            resolvers[k]->getResponseMessage()->header.set_arcount(0);
            resolvers[k]->getResponseMessage()->header.set_qr(1);

            ivec[0].iov_base = &resolvers[k]->getResponseMessage()->header;
            ivec[0].iov_len = 12;
            ivec[1].iov_base = &obj->msg.buf;
            ivec[1].iov_len = obj->msgLen - 12;
            int offset = resolvers[k]->getReply()->getAnswerOffset();
            ivec[2].iov_base = ((u_int8_t*)resolvers[k]->getResponseMessage()) + offset;
            ivec[2].iov_len = resolvers[k]->getResponseMessageSize() - offset;

            hdr.msg_name = &(obj->in);
            hdr.msg_namelen = addrlen;
            hdr.msg_iov = &ivec[0];
            hdr.msg_iovlen = 3;
            hdr.msg_control = NULL;
            hdr.msg_controllen = 0;
            hdr.msg_flags = 0;

            if(sendmsg(obj->fd, &hdr, 0) == -1){
                log->log(1, obj->id, resolvers[k]->getSubid(), "sendto failed: %s", strerror(errno));
            }

        } else {
            if(sendto(obj->fd, resolvers[k]->getResponseMessage(), resolvers[k]->getResponseMessageSize(), 0, (struct sockaddr*)&(obj->in), addrlen) == -1){
                log->log(1, obj->id, resolvers[k]->getSubid(), "sendto failed: %s", strerror(errno));
            }
        }
    }

    return resolvers[k]->getResponseMessageSize();
}

void ResolverThread::dump(ShotgundResolver * resolver, QueueObj * obj, const char * prefix)
{
    const RFC1035_RR * answer = resolver->getReply()->getAnswer();

    float ms = (tv[1].tv_sec - tv[0].tv_sec)*1000 + float((tv[1].tv_usec - tv[0].tv_usec))/1000;

    if(answer != NULL){
        if(answer->type == RFC1035_TYPE_MX){
            log->log(4, obj->id, resolver->getSubid(), "%s[MX %s %d %s] %.2fms", prefix, answer->name, ((RFC1035_MX_RR*)answer)->preference, ((RFC1035_MX_RR*)answer)->exchange, ms);
            return;
        }
        if(answer->type == RFC1035_TYPE_CNAME){
            log->log(4, obj->id, resolver->getSubid(), "%s[CNAME: %s %s] %.2fms", prefix, answer->name, ((RFC1035_CNAME_RR*)answer)->cname, ms);
            return;
        }
        if(answer->type == RFC1035_TYPE_A){
            log->log(4, obj->id, resolver->getSubid(), "%s[A %s %s] %.2fms", prefix, answer->name, inet_ntoa(((RFC1035_A_RR*)answer)->address), ms);
            return;
        }
        if(answer->type == RFC1035_TYPE_AAAA){
            char nsaddr[RFC1035_INET_ADDRSTRLEN];
            if(inet_ntop(RFC1035_AF_INET, &((RFC1035_AAAA_RR*)answer)->address, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                nsaddr[0] = 0;
            }
            log->log(4, obj->id, resolver->getSubid(), "%s[AAAA: %s %s] %.2fms", prefix, answer->name, nsaddr, ms);
            return;
        }
        if(answer->type == RFC1035_TYPE_SOA){
            log->log(4, obj->id, resolver->getSubid(), "%s[SOA: %s %s %d %d %d %d %d] %.2fms",
                answer->name,
                ((RFC1035_SOA_RR*)answer)->mname,
                ((RFC1035_SOA_RR*)answer)->rname,
                ((RFC1035_SOA_RR*)answer)->serial,
                ((RFC1035_SOA_RR*)answer)->refresh,
                ((RFC1035_SOA_RR*)answer)->retry,
                ((RFC1035_SOA_RR*)answer)->expire,
                ((RFC1035_SOA_RR*)answer)->minimum, ms);
            return;
        }
        if(answer->type == RFC1035_TYPE_NS){
            log->log(4, obj->id, resolver->getSubid(), "%s[NS: %s %s] %.2fms", prefix, answer->name, ((RFC1035_NS_RR*)answer)->nsdname, ms);
            return;
        }
        if(answer->type == RFC1035_TYPE_PTR){
            log->log(4, obj->id, resolver->getSubid(), "%s[PTR: %s %s] %.2fms", prefix, answer->name, ((RFC1035_PTR_RR*)answer)->ptrdname, ms);
            return;
        }
        if(answer->type == RFC1035_TYPE_TXT){
            log->log(4, obj->id, resolver->getSubid(), "%s[TXT: %s] %.2fms", prefix, answer->name, ms);
            return;
        }
        if(answer->type == RFC1035_TYPE_HINFO){
            log->log(4, obj->id, resolver->getSubid(), "%s[HINFO: %s '%s' '%s'] %.2fms", prefix, answer->name, ((RFC1035_HINFO_RR*)answer)->cpu?((RFC1035_HINFO_RR*)answer)->cpu:"", ((RFC1035_HINFO_RR*)answer)->os?((RFC1035_HINFO_RR*)answer)->os:"", ms);
            return;
        }
        if(answer->type == RFC1035_TYPE_NAPTR){
            log->log(4, obj->id, resolver->getSubid(), "%s[NAPTR: %s] %.2fms", prefix, answer->name, ms);
            return;
        }
        if(answer->type == RFC1035_TYPE_SRV){
            log->log(4, obj->id, resolver->getSubid(), "%s[SRV: %s] %.2fms", prefix, answer->name, ms);
            return;
        }
    } else {
        switch(resolver->getResponseMessage()->header.get_rcode()){
            case RFC1035_RCODE_NOERROR:
                log->log(4, obj->id, resolver->getSubid(), "Empty Reply: [RCODE: NOERROR] %.2fms", ms);
                break;
            case RFC1035_RCODE_FORMAT:
                log->log(4, obj->id, resolver->getSubid(), "Empty Reply: [RCODE: FORMAT] %.2fms", ms);
                break;
            case RFC1035_RCODE_SERVFAIL:
                log->log(4, obj->id, resolver->getSubid(), "Empty Reply: [RCODE: SERVFAIL] %.2fms", ms);
                break;
            case RFC1035_RCODE_NXDOMAIN:
                log->log(4, obj->id, resolver->getSubid(), "Empty Reply: [RCODE: NXDOMAIN] %.2fms", ms);
                break;
            case RFC1035_RCODE_UNIMPLEMENTED:
                log->log(4, obj->id, resolver->getSubid(), "Empty Reply: [RCODE: UNIMPLEMENTED] %.2fms", ms);
                break;
            case RFC1035_RCODE_REFUSED:
                log->log(4, obj->id, resolver->getSubid(), "Empty Reply: [RCODE: REFUSED] %.2fms", ms);
                break;
            default:
                log->log(4, obj->id, resolver->getSubid(), "Empty Reply: [RCODE UNKNOWN] %.2fms", ms);
        }
    }
}

const char * ResolverThread::getQTypeStr(u_int8_t qtype)
{
    switch(qtype){
            case RFC1035_TYPE_A: return "A";
            case RFC1035_TYPE_NS: return "NS";
            case RFC1035_TYPE_MD: return "MD";
            case RFC1035_TYPE_MF: return "MF";
            case RFC1035_TYPE_CNAME: return "CNAME";
            case RFC1035_TYPE_SOA: return "SOA";
            case RFC1035_TYPE_MB: return "MB";
            case RFC1035_TYPE_MG: return "MG";
            case RFC1035_TYPE_MR: return "MR";
            case RFC1035_TYPE_NULL: return "NULL";
            case RFC1035_TYPE_WKS: return "WKS";
            case RFC1035_TYPE_PTR: return "PTR";
            case RFC1035_TYPE_HINFO: return "HINFO";
            case RFC1035_TYPE_MINFO: return "MINFO";
            case RFC1035_TYPE_MX: return "MX";
            case RFC1035_TYPE_TXT: return "TXT";
            case RFC1035_TYPE_AAAA: return "AAAA";
            case RFC1035_TYPE_AXFR: return "AFXR";
            case RFC1035_TYPE_MAILB: return "MAILB";
            case RFC1035_TYPE_MAILA: return "MAILA";
            case RFC1035_TYPE_ANY: return "ANY";
            case RFC1035_TYPE_NAPTR: return "NAPTR";
            case RFC1035_TYPE_SRV: return "SRV";
        }
    return "-";
}

int ResolverThread::start(pthread_t * thr_id)
{
    //log->log(4, 0,0, "thread::start");

    if(resolvers[0]->initialize() == -1){
        return -1;
    }

    if(pthread_create(thr_id, NULL, threadFunc, this) != 0){
        log->log(1, 0,0, "pthread_create() failed");
        return -1;
    }

    threadSelf = *thr_id;

    return 0;
}

void ResolverThread::stop()
{
    //log->log(4, 0,0, "thread::stop");

    shutdown = true;
    pthread_cancel(threadSelf);
    pthread_join(threadSelf, NULL);
}

const char * ResolverThread::checkSuffix(const char * name, const char * suffix)
{
    size_t nlen = strlen(name);
    size_t slen = strlen(suffix);

    if(nlen < slen){
        return NULL;
    }

    if(suffix[slen-1] == '.')
        slen--;

    if(!strncasecmp(&name[nlen-slen], suffix, slen)){
        return &name[nlen-slen];
    }

    return NULL;
}

/*
int ResolverThread::sendICMPUnreach(const sockaddr_in * dest)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int fd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    int len = 8;
    struct icmp * icmp = (struct icmp*)malloc(len);

    icmp->icmp_type = ICMP_DEST_UNREACH;
    icmp->icmp_code = ICMP_PORT_UNREACH;
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = in_cksum((u_short*)icmp, len);

    if(fd == -1){
        log->log(1,0,0, "socket(SOCK_RAW) failed: %s", strerror(errno));
        free(icmp);
        return -1;
    }

    if(sendto(fd, icmp, 8, 0, (struct sockaddr*)dest, addrlen) == -1){
        log->log(1,0,0, "sendto(icmp) failed: %s", strerror(errno));
        free(icmp);   close(fd);
        return -1;
    }

    free(icmp);   close(fd);

    return 0;
}
*/

char * ResolverThread::jRewrite(char * qname)
{
    char * p = strrchr(qname, '*');
    char * z = p;

    while(*z){ //find zone start
        if(isalpha(*z)) {
            break;
        }
        z++;
    }

    char * i = z-1;

    char * tmp = (char*)alloca(i-p);
    int k = 0;
    while(i>p){
        if(isdigit(*i)){
            tmp[k++] = *i;
        }
        i--;
    }
    tmp[k] = 0;

    while((*p++ = *tmp++));

    *(p-1) = '.';

    while((*p++ = *z++));

    return qname;
}

char * ResolverThread::iRewrite(char * qname)
{
    char * p = strchr(qname, '*');
    char * z = p;

    while(*z){ //find zone start
        if(isalpha(*z)) {
            break;
        }
        z++;
    }

    char * tmp = (char*)alloca(strlen(qname) + 1);
    int k = 0;
    char * i = p + 2;
    while(i<z){
        tmp[k++] = *i++;
    }
    i = p-1;
    while(i>=qname){
        if(isdigit(*i)){
            tmp[k++] = *i;
        }
        i--;
    }

    tmp[k++] = '.';

    while(*z){
        tmp[k++] = *z++;
    }

    tmp[k] = 0;

    strncpy(qname, tmp, k+1);

    return qname;
}

