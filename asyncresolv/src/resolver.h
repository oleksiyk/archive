/*  Copyright (C) 2003 Aleksey Krivoshey <krivoshey@users.sourceforge.net>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


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

    static struct RFC1035_sockaddr_in nameservers[10];
    static u_int8_t nameserversCount;
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
    const AsyncDNSReply * getReply() const;
    void cleanup();

    int wait(int mlsec = 3000);
    int timedout();
    struct pollfd * getPollfd();

    ErrorCode getErrorCode()const { return errorCode; }
};

#endif

