/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_SSL_H
#define PROBED_SSL_H

#include "decl.h"

#define TLS_CLIENT_VERIFY_NONE              0
#define TLS_CLIENT_VERIFY_OPTIONAL          1
#define TLS_CLIENT_VERIFY_REQUIRE           2

class ProbedSSL
{
private:
    static ProbedSSL *self;
    const Log * log;
    SSL_CTX *ctx;
    pthread_mutex_t *lock_mutexes;

    int verifyClient;
    int verifyClientDepth;

    static void locking_function(int mode, int n, const char * file, int line);
    static unsigned long id_function(void);
    static int cb_SSLVerify(int ok, X509_STORE_CTX *ctx);

    STACK_OF(X509_NAME) * loadCAList(const char *CAfile, const char *CApath);

public:
    ProbedSSL();
    virtual ~ProbedSSL();

    virtual int initialize();
    virtual void finish();
    
    SSL_CTX * getSSL_CTX()const{ return ctx; }

    //virtual InSmtpExtension * getInSmtpExtension(InSmtpConnection * _connection);

};


#endif
