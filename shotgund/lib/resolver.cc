/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */


#include "rfc1035.h"

/* initialize static fields */
/*
u_int8_t AsyncDNSResolver::nameserversCount = 0;

RFC1035_sockaddr_in AsyncDNSResolver::nameservers[];
*/

char * AsyncDNSResolver::defaultdomain = NULL;

pthread_mutex_t AsyncDNSResolver::lock = PTHREAD_MUTEX_INITIALIZER;
/* ------ */

AsyncDNSResolver::AsyncDNSResolver():
    mempool(NULL), responseMessage(NULL),
    queryMessageSize(sizeof(RFC1035_MessageHeader)), responseMessageSize(0), tcpResponseSize(0),
    nameserversCount(0), // defaultdomain(NULL),
    udpsockfd(-1), tcpsockfd(-1), tcpSentBytes(0), tcpRecvBytes(0),
    curns(0), cnameTranslations(0), reply(NULL),
    status(SEND_UDP), errorCode(DNS_ERR_SUCCESS)
{
}

AsyncDNSResolver::~AsyncDNSResolver()
{
    if(reply != NULL){
        reply->cleanup();
    }

    delete reply;

    free(defaultdomain);

    if(udpsockfd != -1){
        close(udpsockfd);
    }
    if(tcpsockfd != -1){
        close(tcpsockfd);
    }

    delete mempool;
}

int AsyncDNSResolver::read_resolv_conf()
{
    pthread_mutex_lock(&lock);

    if(AsyncDNSResolver::nameserversCount == 0){

        FILE * fp=fopen("/etc/resolv.conf", "r");
        char buf[512];
        char * q;

        if(fp == NULL){
            error("fopen(/etc/resolv.conf) failed: %s", strerror(errno));
            errorCode = DNS_ERR_HARD;
            pthread_mutex_unlock(&lock);
            return -1;
        }

        char * p;
        struct RFC1035_in_addr sin_addr;
        socklen_t addrlen = sizeof(struct RFC1035_sockaddr_in);
        while (fgets(buf, 512, fp)){
            buf[511] = 0;
            for (p=buf; *p; p++){
                if(isspace(*p))
                    break;
                *p=tolower(*p);
            }

            if (*p) *p++=0;

            if (!strcasecmp(buf, "domain") && !AsyncDNSResolver::defaultdomain){
                while (p && isspace(*p)) p++;
                for (q=p; *q && !isspace(*q); q++);
                *q=0;
                AsyncDNSResolver::defaultdomain = strdup(p);
                continue;
            }

            if (!strcasecmp(buf, "nameserver") && AsyncDNSResolver::nameserversCount<10){
                while (*p && isspace(*p)) p++;
                for (q=p; *q && !isspace(*q); q++);
                *q=0;

                memset(&AsyncDNSResolver::nameservers[AsyncDNSResolver::nameserversCount], 0, addrlen);

                AsyncDNSResolver::nameservers[AsyncDNSResolver::nameserversCount].RFC1035_sin_family = RFC1035_AF_INET;
                AsyncDNSResolver::nameservers[AsyncDNSResolver::nameserversCount].RFC1035_sin_port = htons(53);
#ifdef HAVE_IPV6
                if(!strchr(p, ':')){ // ipv4?
                    strcpy(buf, "::ffff:");
                    memmove(buf+7, p, strlen(p)+1);
                    p = buf;
                }
#endif
                int k = inet_pton(RFC1035_AF_INET, p, &sin_addr);
                memcpy(&AsyncDNSResolver::nameservers[AsyncDNSResolver::nameserversCount++].RFC1035_sin_addr,
                    &sin_addr,sizeof(struct RFC1035_in_addr));

                if(k <= 0){
                    if(k == 0){
                        error("inet_pton(%s) failed: Invalid network address", p);
                    } else {
                        error("inet_pton(%s) failed: %s", p, strerror(errno));
                    }
                    AsyncDNSResolver::nameserversCount--;
                }
            }
        }

        fclose(fp);

        if(AsyncDNSResolver::nameserversCount == 0){
            error("No nameservers found in /etc/resolv.conf");
            pthread_mutex_unlock(&lock);
            return -1;
        }
    }

    pthread_mutex_unlock(&lock);

    return 0;
}

int AsyncDNSResolver::initialize()
{
    mempool = new AsyncDNSMemPool(1024);

    if(mempool->initialize() == -1){
        fprintf(stderr, "Out of memory\n");
        return -1;
    }

    /*
    if(read_resolv_conf() == -1){
        return -1;
    }
    */

    reply = new AsyncDNSReply(mempool);

    queryMessage.header.id = getRandId();


    return 0;
}

u_int16_t AsyncDNSResolver::getRandId()
{
    u_int16_t id = 0;

    int fd = open("/dev/urandom", O_RDONLY);
    if(fd != -1){
        read(fd, &id, 2);
        close(fd);
    }

    return id;
}

void AsyncDNSResolver::error(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    size_t len = strlen(fmt) + 30;
    if(len > 256)
        len = 256;
    char * buf = (char*)mempool->alloc(len);

    (void) snprintf(buf, len, "AsyncDNSResolver: %s\n", fmt);
    buf[len-1] = 0;

    (void) vfprintf(stderr, buf, args);

    va_end(args);
}

void AsyncDNSResolver::prepareQueryInternal(u_int8_t _opcode, u_int8_t _recursive)
{
    queryMessageSize = sizeof(RFC1035_MessageHeader);

    queryMessage.header.flags_lo = queryMessage.header.flags_hi = 0;

    queryMessage.header.id = getRandId();
    queryMessage.header.set_opcode(_opcode);
    queryMessage.header.set_rd(_recursive);

    queryMessage.header.qdcount_hi = queryMessage.header.qdcount_lo = 0;
    queryMessage.header.ancount_hi = queryMessage.header.ancount_lo = 0;
    queryMessage.header.nscount_hi = queryMessage.header.nscount_lo = 0;
    queryMessage.header.arcount_hi = queryMessage.header.arcount_lo = 0;

    status = SEND_UDP;
}

int AsyncDNSResolver::prepareQuery(u_int8_t _opcode, u_int8_t _recursive)
{
    resolveQueriesCallCounter = 1;

    cleanup();

    if(openUDP() == -1){
        return -1;
    }

    curns = 0;

    prepareQueryInternal(_opcode, _recursive);

    return 0;
}

int AsyncDNSResolver::addQuery(const char * _domainName, u_int16_t _qtype, u_int16_t _qclass)
{
    //NOTE: must add domain name compresion

    u_int8_t * qname = &((u_int8_t*)&queryMessage)[queryMessageSize];

    const char * p = _domainName;
    u_int8_t c = 0;
    size_t n = 0, k = 1;


    while(*p) {
        if((queryMessageSize+k) >= 512){
            error("cannot add query: message size (512) exceeded");
            errorCode = DNS_ERR_BADQUERY;
            return -1;
        }
        if(k > RFC1035_MAX_NAMESIZE){
            error("invalid domain name '%s': name too long", _domainName);
            errorCode = DNS_ERR_BADQUERY;
            return -1;
        }
        if(*p != '.'){
            c++;
            qname[k++] = (u_int8_t)*p;
        } else {
            if(c == 0){ //invalid domainname: empty label
                break;
            }
            if(c > RFC1035_MAX_LABELSIZE){
                break;
            }
            qname[n] = c;
            c = 0;
            n = k++;
        }
        p++;
        if( !*p && n == 0 && defaultdomain){
            qname[n] = c;
            c = 0;
            n = k++;
            p = defaultdomain;
        }
    }
    if(c == 0){
        error("invalid domain name '%s': empty label", _domainName);
        errorCode = DNS_ERR_BADQUERY;
        return -1;
    }
    if(c > RFC1035_MAX_LABELSIZE){
        error("invalid domain name '%s': label too long", _domainName);
        errorCode = DNS_ERR_BADQUERY;
        return -1;
    }
    qname[n] = c;
    qname[k++] = 0;
    qname[k++] = _qtype >> 8;
    qname[k++] = (u_int8_t)_qtype;
    qname[k++] = _qclass >> 8;
    qname[k++] = (u_int8_t)_qclass;

    qtype = _qtype;

    queryMessageSize += k;

    queryMessage.header.set_qdcount(queryMessage.header.get_qdcount() + 1);

    return 0;
}

int AsyncDNSResolver::openUDP()
{
    udpsockfd = socket(RFC1035_PF_INET, SOCK_DGRAM, 0);
    if(udpsockfd == -1){
        error("socket(SOCK_DGRAM) failed: %s", strerror(errno));
        errorCode = DNS_ERR_HARD;
        return -1;
    }
    if(fcntl(udpsockfd, F_SETFL, O_NONBLOCK) == -1){
        error("setting O_NONBLOCK on UDP socket failed: %s", strerror(errno));
        errorCode = DNS_ERR_HARD;
        return -1;
    }

    return 0;
}

int AsyncDNSResolver::sendUDP(struct RFC1035_sockaddr_in * to)
{
    socklen_t addrlen = sizeof(struct RFC1035_sockaddr_in);

    if(sendto(udpsockfd, &queryMessage, queryMessageSize, 0, (struct sockaddr*)to, addrlen) == -1){
        if(errno != EAGAIN){
            char nsaddr[RFC1035_INET_ADDRSTRLEN];
            if(inet_ntop(RFC1035_AF_INET, &to->RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                nsaddr[0] = 0;
            }
            error("cannot contact nameserver at %s: UDP sendto() failed: %s", nsaddr, strerror(errno));
            errorCode = DNS_ERR_HARD;
            return -1;
        } else {
            return -EAGAIN;
        }
    }
    return 0;
}

int AsyncDNSResolver::recvUDP()
{
    int responseLen = 0;
    struct RFC1035_sockaddr_in from;
    socklen_t addrlen = sizeof(struct RFC1035_sockaddr_in);

    if(responseMessageSize < sizeof(AsyncDNSMessage)){
        responseMessage = (AsyncDNSMessage*)mempool->alloc(sizeof(AsyncDNSMessage));
        responseMessageSize = sizeof(AsyncDNSMessage);
    }

    responseLen = recvfrom(udpsockfd, responseMessage, responseMessageSize, 0, (struct sockaddr*)&from, &addrlen);
    if(responseLen == -1){
        if(errno != EAGAIN){
            error("UDP recvfrom() failed: %s", strerror(errno));
            errorCode = DNS_ERR_HARD;
            return -1;
        } else {
            return -EAGAIN;
        }
    }
    for(size_t i = 0; i<nameserversCount; i++){
#ifdef HAVE_IPV6
        if(!memcmp(nameservers[i].sin6_addr.s6_addr,from.sin6_addr.s6_addr,sizeof(from.sin6_addr.s6_addr))){
#else
        if(nameservers[i].sin_addr.s_addr == from.sin_addr.s_addr){
#endif
            return responseLen;
        }
    }
    char nsaddr[RFC1035_INET_ADDRSTRLEN];
    if(inet_ntop(RFC1035_AF_INET, &from.RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
        nsaddr[0] = 0;
    }
    error("UDP received reply from unknown nameserver [%s], reply ignored", nsaddr);
    return -EAGAIN;
}

void AsyncDNSResolver::closeTCP()
{
    tcpRecvBytes = 0;
    tcpSentBytes = 0;

    if(tcpsockfd != -1){
        close(tcpsockfd);
    }

    tcpsockfd = -1;
}

int AsyncDNSResolver::openTCP(struct RFC1035_sockaddr_in * to)
{
    if(tcpsockfd != -1){

        int sock_err = -1;
        socklen_t sock_err_len = sizeof(sock_err);

        if(getsockopt(tcpsockfd, SOL_SOCKET, SO_ERROR, &sock_err, &sock_err_len) == -1){
            error("getsockopt() failed: %s", strerror(errno));
            errorCode = DNS_ERR_HARD;
            return -1;
        }
        if(sock_err == 0){
            return 0;
        } else {
            char nsaddr[RFC1035_INET_ADDRSTRLEN];
            if(inet_ntop(RFC1035_AF_INET, &to->RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                nsaddr[0] = 0;
            }
            error("TCP connect() to %s failed: %s", nsaddr, strerror(errno));
            errorCode = DNS_ERR_HARD;
            return -1;
        }
    }

    socklen_t addrlen = sizeof(struct RFC1035_sockaddr_in);

    tcpSentBytes = 0;
    tcpRecvBytes = 0;

    tcpsockfd = socket(RFC1035_PF_INET, SOCK_STREAM, 0);

    if(tcpsockfd == -1){
        error("socket(SOCK_STREAM) failed: %s", strerror(errno));
        errorCode = DNS_ERR_HARD;
        return -1;
    }
    if(fcntl(tcpsockfd, F_SETFL, O_NONBLOCK) == -1){
        error("setting O_NONBLOCK on TCP socket failed: %s", strerror(errno));
        errorCode = DNS_ERR_HARD;
        return -1;
    }

    if(connect(tcpsockfd, (struct sockaddr*)to, addrlen) == -1){
        if(errno == EINPROGRESS){
            return -EAGAIN;
        } else {
            char nsaddr[RFC1035_INET_ADDRSTRLEN];
            if(inet_ntop(RFC1035_AF_INET, &to->RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                nsaddr[0] = 0;
            }
            error("TCP connect() to %s failed: %s", nsaddr, strerror(errno));
            errorCode = DNS_ERR_HARD;
            return -1;
        }
    }
    return 0;
}

int AsyncDNSResolver::sendTCP(struct RFC1035_sockaddr_in * to)
{
    int i = 0;
    u_int16_t length = htons(queryMessageSize);

    struct iovec * vec = NULL;
    int vec_count = 2;

    if(tcpSentBytes < 2){
        tcpSendIoVec[0].iov_base = (((u_int8_t*)&length)+tcpSentBytes);
        tcpSendIoVec[0].iov_len = 2-tcpSentBytes;
        tcpSendIoVec[1].iov_base = &queryMessage;
        tcpSendIoVec[1].iov_len = queryMessageSize;
        vec = &tcpSendIoVec[0];
        vec_count = 2;
    } else {
        tcpSendIoVec[1].iov_base = ((u_int8_t*)&queryMessage) + tcpSentBytes - 2;
        tcpSendIoVec[1].iov_len = queryMessageSize-tcpSentBytes+2;
        vec = &tcpSendIoVec[1];
        vec_count = 1;
    }
    while( (i = writev(tcpsockfd, vec, vec_count)) > 0){
        tcpSentBytes += i;
        if(tcpSentBytes == (queryMessageSize+2)){
            return 0;
        }
        if(tcpSentBytes < 2){
            tcpSendIoVec[0].iov_base = (((u_int8_t*)&length)+tcpSentBytes);
            tcpSendIoVec[0].iov_len = 2-tcpSentBytes;
            tcpSendIoVec[1].iov_base = &queryMessage;
            tcpSendIoVec[1].iov_len = queryMessageSize;
            vec = &tcpSendIoVec[0];
            vec_count = 2;
        } else {
            tcpSendIoVec[1].iov_base = ((u_int8_t*)&queryMessage) + tcpSentBytes - 2;
            tcpSendIoVec[1].iov_len = queryMessageSize-tcpSentBytes+2;
            vec = &tcpSendIoVec[1];
            vec_count = 1;
        }
    }
    if(i == -1){
        if(errno == EAGAIN){
            return -EAGAIN;
        } else {
            char nsaddr[RFC1035_INET_ADDRSTRLEN];
            if(inet_ntop(RFC1035_AF_INET, &to->RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                nsaddr[0] = 0;
            }
            error("cannot contact nameserver %s: TCP writev() failed: %s", nsaddr, strerror(errno));
            errorCode = DNS_ERR_HARD;
            return -1;
        }
    }
    return 0;
}

int AsyncDNSResolver::recvTCP(struct RFC1035_sockaddr_in * from)
{
    int i = 0;

    u_int16_t length = 0;

    if(tcpRecvBytes < 2){ //recv length
        while( (i = recv(tcpsockfd, ((u_int8_t*)&length)+tcpRecvBytes, 2-tcpRecvBytes , 0)) > 0){
            tcpRecvBytes += i;
            if(tcpRecvBytes == 2){
                if(ntohs(length) > responseMessageSize){
                    responseMessage = (AsyncDNSMessage*)mempool->alloc(ntohs(length));
                }
                responseMessageSize = ntohs(length);
                break;
            }
        }
    }
    if(i == -1){
        if(errno == EAGAIN){
            return -EAGAIN;
        } else {
            char nsaddr[RFC1035_INET_ADDRSTRLEN];
            if(inet_ntop(RFC1035_AF_INET, &from->RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                nsaddr[0] = 0;
            }
            error("cannot contact nameserver %s: TCP recv() failed: %s", nsaddr, strerror(errno));
            errorCode = DNS_ERR_HARD;
            return -1;
        }
    }

    void * buf = ((u_int8_t*)responseMessage)+tcpRecvBytes-2;
    while( (i = recv(tcpsockfd, buf, responseMessageSize-tcpRecvBytes+2, 0)) > 0){
        tcpRecvBytes += i;
        if(tcpRecvBytes == (responseMessageSize+2)){
            return responseMessageSize;
        }
        buf = ((u_int8_t*)responseMessage)+tcpRecvBytes-2;
    }
    if(i == -1){
        if(errno == EAGAIN){
            return -EAGAIN;
        } else {
            char nsaddr[RFC1035_INET_ADDRSTRLEN];
            if(inet_ntop(RFC1035_AF_INET, &from->RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                nsaddr[0] = 0;
            }
            error("cannot contact nameserver %s: TCP recv() failed: %s", nsaddr, strerror(errno));
            errorCode = DNS_ERR_HARD;
            return -1;
        }
    }
    return 0;
}

int AsyncDNSResolver::resolveQueries()
{
    if(!queryMessage.header.get_qdcount()){
        errorCode = DNS_ERR_BADQUERY;
        error("AsyncDNSResolver::resolveQueries(): no queries to resolve");
        return -1;
    }

    int responseLen = 0;
    int k = 0;
    for(size_t ns = curns; ns < nameserversCount; ns++){

        if(status == SEND_UDP){
            k = sendUDP(&nameservers[ns]);

            if( k == -EAGAIN ){
                curns = ns;
                return EAGAIN;
            } else if(k == -1){
                continue;
            }
            status = RECV_UDP;
        }

        if(status == RECV_UDP){
            k = recvUDP();

            if( k == -EAGAIN ) {
                curns = ns;
                return EAGAIN;
            } else if(k == -1 || k == 0){
                status = SEND_UDP;
                continue;
            }
            responseLen = k;
        } else { //TCP query

            if (status == OPEN_TCP){
                k = openTCP(&nameservers[ns]);
                if( k == -EAGAIN ){
                    curns = ns;
                    return EAGAIN;
                } else if(k == -1){
                    closeTCP();
                    status = SEND_UDP;
                    continue;
                }
                status = SEND_TCP;
            }
            if (status == SEND_TCP){
                k = sendTCP(&nameservers[ns]);

                if( k == -EAGAIN ){
                    curns = ns;
                    return EAGAIN;
                } else if(k == -1){
                    closeTCP();
                    status = SEND_UDP;
                    continue;
                }
                status = RECV_TCP;
            }
            if(status == RECV_TCP){
                k = recvTCP(&nameservers[ns]);

                if( k == -EAGAIN ) {
                    curns = ns;
                    return EAGAIN;
                } else if(k == -1 || k == 0){
                    closeTCP();
                    status = SEND_UDP;
                    continue;
                }
                closeTCP();
                responseLen = k;
            }
        }

        if(responseLen){
            if((size_t)responseLen < sizeof(RFC1035_MessageHeader)){
                char nsaddr[RFC1035_INET_ADDRSTRLEN];
                if(inet_ntop(RFC1035_AF_INET, &nameservers[ns].RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                    nsaddr[0] = 0;
                }
                error("Response message too small (nameserver %s)", nsaddr);
                errorCode = DNS_ERR_BADRESPONSE; status = SEND_UDP; responseLen = 0;
                continue;
            }
            if(responseMessage->header.id != queryMessage.header.id || responseMessage->header.get_opcode() != queryMessage.header.get_opcode()){
                char nsaddr[RFC1035_INET_ADDRSTRLEN];
                if(inet_ntop(RFC1035_AF_INET, &nameservers[ns].RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                    nsaddr[0] = 0;
                }
                error("Response message error: response ID not matched query ID (nameserver %s)", nsaddr);
                errorCode = DNS_ERR_BADRESPONSE; status = SEND_UDP; responseLen = 0;
                continue;
            }
            if(responseMessage->header.get_qr() != 1){
                char nsaddr[RFC1035_INET_ADDRSTRLEN];
                if(inet_ntop(RFC1035_AF_INET, &nameservers[ns].RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                    nsaddr[0] = 0;
                }
                error("Response message error: query flag set in response message (nameserver %s)", nsaddr);
                errorCode = DNS_ERR_BADRESPONSE; status = SEND_UDP; responseLen = 0;
                continue;
            }
            /*
            if(responseMessage->header.get_ra() == 0){
                char nsaddr[RFC1035_INET_ADDRSTRLEN];
                if(inet_ntop(RFC1035_AF_INET, &nameservers[ns].RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                    nsaddr[0] = 0;
                }
                error("DNS server %s does not support recursive queries", nsaddr);

                errorCode = DNS_ERR_BADDNS; status = SEND_UDP; responseLen = 0;
                continue;
            }
            */
            if(responseMessage->header.get_tc()){
                char nsaddr[RFC1035_INET_ADDRSTRLEN];
                if(inet_ntop(RFC1035_AF_INET, &nameservers[ns].RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                    nsaddr[0] = 0;
                }
                error("Response message is truncated, retrying by TCP (nameserver %s)", nsaddr);
                status = OPEN_TCP; responseLen = 0; tcpRecvBytes = 0; tcpSentBytes = 0;
                curns--;
                continue;
            }
            if(reply->addparse(responseMessage, responseLen) == -1){
                char nsaddr[RFC1035_INET_ADDRSTRLEN];
                if(inet_ntop(RFC1035_AF_INET, &nameservers[ns].RFC1035_sin_addr, nsaddr, RFC1035_INET_ADDRSTRLEN) == NULL){
                    nsaddr[0] = 0;
                }
                error("Response message parse error (nameserver %s)", nsaddr);
                errorCode = DNS_ERR_BADRESPONSE;  status = SEND_UDP; responseLen = 0;
                reply->cleanup();
                responseMessage = NULL;
                responseMessageSize = 0;
                continue;
            }
            break; //ok, response parsed
        }
    }

    //queryMessage.header.set_qdcount(0);

    if(responseLen == 0){
        //error("Failed to execute query");
        //reply->cleanup();
        return -1;
    }

    /*
    if(resolveQueriesCallCounter){

        resolveQueriesCallCounter--;

        return checkReply();
    }
    */

    return 0;
}

int AsyncDNSResolver::checkReply()
{
    const RFC1035_RR * answer = NULL;
    size_t i = 0;

    while((answer = reply->getAnswer(i++)) != NULL){
        /* if there is CNAME in answer then resolve it */
        if(answer->type == RFC1035_TYPE_CNAME && qtype != RFC1035_TYPE_CNAME && !((RFC1035_CNAME_RR*)answer)->resolver_checked){
            size_t k = 0;
            bool alreadyResolved = false;
            const RFC1035_RR * checkanswer = NULL;
            while((checkanswer = reply->getRR(k++)) != NULL){
                /* if we already have A_RR for this CNAME then skip it */
                if(checkanswer->type == RFC1035_TYPE_A){
                    if(!strcasecmp(checkanswer->name, ((RFC1035_CNAME_RR*)answer)->cname)){
                        alreadyResolved = true;
                        break;
                    }
                }
                /* circular CNAME reference */
                if(checkanswer->type == RFC1035_TYPE_CNAME && checkanswer != answer){
                    if(!strcasecmp(((RFC1035_CNAME_RR*)checkanswer)->cname, answer->name)){
                        ((RFC1035_CNAME_RR*)checkanswer)->resolver_checked = 1;
                        alreadyResolved = true;
                        break;
                    }
                }
            }
            ((RFC1035_CNAME_RR*)answer)->resolver_checked = 1;
            if(cnameTranslations >= RFC1035_MAX_CNAME_TRANSLATIONS){ //MAXIMUM number of CNAME tranlsations exceeded
                continue;
            }
            cnameTranslations++;
            if(!alreadyResolved){
                resolveQueriesCallCounter++;
                prepareQueryInternal(RFC1035_OPCODE_QUERY, RFC1035_RESOLVE_RECURSIVE);
                if(addQuery(((RFC1035_CNAME_RR*)answer)->cname, RFC1035_TYPE_A, RFC1035_CLASS_IN) == -1){
                    reply->cleanup();
                    return -1;
                }
                return resolveQueries();
            }
        } else if(answer->type == RFC1035_TYPE_MX  && !((RFC1035_MX_RR*)answer)->resolver_checked){
            /* if there is MX exchange that response doesn't contain A RR for, then resolve it */
            bool alreadyResolved = false;
            const RFC1035_RR * checkanswer = reply->getLastRRByType(RFC1035_TYPE_A);
            while(checkanswer){
                if(!strcasecmp(checkanswer->name, ((RFC1035_MX_RR*)answer)->exchange)){
                    alreadyResolved = true;
                    break;
                }
                checkanswer = checkanswer->prevRR;
            }
            ((RFC1035_MX_RR*)answer)->resolver_checked = 1;
            if(!alreadyResolved){
                resolveQueriesCallCounter++;
                prepareQueryInternal(RFC1035_OPCODE_QUERY, RFC1035_RESOLVE_RECURSIVE);
                if(addQuery(((RFC1035_MX_RR*)answer)->exchange, RFC1035_TYPE_A, RFC1035_CLASS_IN) == -1){
                    reply->cleanup();
                    return -1;
                }
                return resolveQueries();
            }
        }
    }
    return 0;
}

void AsyncDNSResolver::cleanup()
{
    if(udpsockfd != -1){
        close(udpsockfd);
        udpsockfd = -1;
    }

    closeTCP();

    curns = 0;
    status = SEND_UDP;
    responseMessage = NULL;
    responseMessageSize = 0;
    tcpSentBytes = 0;
    tcpRecvBytes = 0;

    if(reply != NULL){
        reply->cleanup();
    }

    mempool->free();

    cnameTranslations = 0;

    errorCode = DNS_ERR_SUCCESS;
}

int AsyncDNSResolver::wait(int mlsec)
{
    int i = poll(getPollfd(), 1, mlsec);

    if(i == -1){
        error("poll() failed: %s", strerror(errno));
        errorCode = DNS_ERR_HARD;
        return -1;
    }
    if(i == 0){
        return EAGAIN;
    }
    /*
    if(fdset.revents & fdset.events){
        return 0;
    }
    if( fdset.revents & (POLLERR | POLLHUP | POLLNVAL)){
        return -1;
    }
    */
    return 0;
}

int AsyncDNSResolver::timedout()
{
    curns++;
    closeTCP();
    status = SEND_UDP;

    if(curns == nameserversCount){
        errorCode = DNS_ERR_HARD;
        error("Failed to execute query: no responses received within timeout");
        return -1;
    }
    return EAGAIN;
}

struct pollfd * AsyncDNSResolver::getPollfd()
{
    if(status == SEND_UDP || status == RECV_UDP){
        fdset.fd = udpsockfd;
    } else {
        fdset.fd = tcpsockfd;
    }

    if(status == SEND_UDP || status == SEND_TCP || status == OPEN_TCP){
        fdset.events = POLLOUT;
    } else {
        fdset.events = POLLIN;
    }

    return &fdset;
}

AsyncDNSReply * AsyncDNSResolver::getReply()
{
    return reply;
    /*
    if(reply->getHeader()){
        return reply;
    }
    return NULL;
    */
}

int AsyncDNSResolver::setNameserver(const char * addr, int port)
{
    struct RFC1035_in_addr sin_addr;
    socklen_t addrlen = sizeof(struct RFC1035_sockaddr_in);

    memset(&nameservers[0], 0, addrlen);

    nameservers[0].RFC1035_sin_family = RFC1035_AF_INET;
    nameservers[0].RFC1035_sin_port = htons(port);

    int k = inet_pton(RFC1035_AF_INET, addr, &sin_addr);
    memcpy(&nameservers[0].RFC1035_sin_addr,
        &sin_addr,sizeof(struct RFC1035_in_addr));

    if(k <= 0){
        if(k == 0){
            error("inet_pton(%s) failed: Invalid network address", addr);
        } else {
            error("inet_pton(%s) failed: %s", addr, strerror(errno));
        }
        return -1;
    }

    nameserversCount = 1;

    return 0;
}


