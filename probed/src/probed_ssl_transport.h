/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_SSL_TRANSPORT_H
#define PROBED_SSL_TRANSPORT_H

#include "decl.h"

class ProbedSSLTransport
{
public:
    enum retcodes {
        OK = 0,
        WOULDBLOCK = -1,
        FAILED = -2,
        CONN_CLOSED = -3
    };
private:
    const Log * log;
    SSL_CTX * ctx;
    SSL * ssl;
    struct pollfd * fdset;
    ProbedThread * thread;
    int timeout;

    void printPeerCertificate();

    void cleanup();

public:
    int initialize();
    int shutdown(bool immediate = false);

    SSL * getConnectionContext()const{ return ssl; }

    virtual ssize_t write(const void *buf, size_t count);
    virtual ssize_t read(void *buf, size_t count);

    ProbedSSLTransport(ProbedThread * _thread, int _timeout);
    virtual ~ProbedSSLTransport();
};

#endif
