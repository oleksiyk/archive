/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef ASYNC_DNS_RESOLVER_H
#define ASYNC_DNS_RESOLVER_H

#include "rfc1035.h"

class AsyncDNSResolver
{
protected:
    AsyncDNSMemPool * mempool;
private:
    AsyncDNSMessage queryMessage;
    AsyncDNSMessage * responseMessage;
    u_int16_t queryMessageSize;
    u_int16_t responseMessageSize;
    u_int16_t tcpResponseSize;

    //static struct RFC1035_sockaddr_in nameservers[10];
    struct RFC1035_sockaddr_in nameservers[1];
    u_int8_t nameserversCount;
    static char * defaultdomain;
    static pthread_mutex_t lock;

    int udpsockfd;
    int tcpsockfd;
    u_int16_t tcpSentBytes;
    u_int16_t tcpRecvBytes;
    struct iovec tcpSendIoVec[2];
    u_int16_t qtype; //current query type
    size_t curns;
    int resolveQueriesCallCounter;
    u_int8_t cnameTranslations;
    struct pollfd fdset;

    AsyncDNSReply * reply;

    enum Status {
        SEND_UDP,
        RECV_UDP,
        OPEN_TCP,
        SEND_TCP,
        RECV_TCP
    } status;

    enum ErrorCode {
        DNS_ERR_SUCCESS,
        DNS_ERR_HARD,
        DNS_ERR_SOFT,
        DNS_ERR_BADDNS,
        DNS_ERR_BADQUERY,
        DNS_ERR_BADRESPONSE
    } errorCode;

    int read_resolv_conf();
    u_int16_t getRandId();
    int openUDP();
    int sendUDP(struct RFC1035_sockaddr_in * to);
    int openTCP(struct RFC1035_sockaddr_in * to);
    int sendTCP(struct RFC1035_sockaddr_in * to);
    int recvTCP(struct RFC1035_sockaddr_in * from);
    int recvUDP();
    inline void closeTCP();
    void prepareQueryInternal(u_int8_t _opcode, u_int8_t _recursive);
    int checkReply();

protected:
    virtual void error(const char * fmt, ...); //can be overloaded in user class

public:
    AsyncDNSResolver();
    virtual ~AsyncDNSResolver();

    int initialize();

    int prepareQuery(u_int8_t _opcode, u_int8_t _recursive);
    int addQuery(const char * _domainName, u_int16_t _qtype, u_int16_t _qclass);
    int resolveQueries();

    AsyncDNSReply * getReply();
    AsyncDNSMessage * getResponseMessage() { return responseMessage; }
    int getResponseMessageSize() { return responseMessageSize; }
    AsyncDNSMessage * getQueryMessage() { return &queryMessage; }
    int getQueryMessageSize() { return queryMessageSize; }

    void cleanup();

    int wait(int mlsec = 3000);
    int timedout();
    struct pollfd * getPollfd();

    int setNameserver(const char * addr, int port);

    ErrorCode getErrorCode()const { return errorCode; }
};

#endif

