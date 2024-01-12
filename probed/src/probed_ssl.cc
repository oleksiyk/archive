/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#include "probed_ssl.h"

typedef unsigned long (*T_OPENSSL_ID_CALLBACK_FUNC)();
typedef void (*T_OPENSSL_LOCKING_CALLBACK_FUNC)(int, int, const char *, int);

ProbedSSL * ProbedSSL::self = NULL;

static int m_x509_cmp(X509_NAME **a, X509_NAME **b)
{
    return (X509_NAME_cmp(*a, *b));
}

void ProbedSSL::locking_function(int mode, int n, const char * file, int line)
{
    if (mode & CRYPTO_LOCK){
        pthread_mutex_lock(&(self->lock_mutexes[n]));
    } else {
        pthread_mutex_unlock(&(self->lock_mutexes[n]));
    }
}

unsigned long ProbedSSL::id_function()
{
    return ((unsigned long)pthread_self());
}

int ProbedSSL::cb_SSLVerify(int ok, X509_STORE_CTX *ctx)
{
    X509 *cert;
    int errnum;
    int errdepth;

    // Get verify ingredients
    cert = X509_STORE_CTX_get_current_cert(ctx);
    errnum = X509_STORE_CTX_get_error(ctx);
    errdepth = X509_STORE_CTX_get_error_depth(ctx);

    // Log verification information
    char *subj, *issuer;
    subj  = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);
    issuer = X509_NAME_oneline(X509_get_issuer_name(cert),  NULL, 0);
    self->log->log(2, "Certificate Verification: depth: %d, subject: %s, issuer: %s", errdepth,
        subj != NULL ? subj : "--unknown--",
        issuer != NULL ? issuer : "--unknown--");

    free(subj); free(issuer);

    /*
    // CRL checks
    if (ok) {
        ok = cb_SSLVerify_CRL(ok, ctx, s);
        if (!ok)
            errnum = X509_STORE_CTX_get_error(ctx);
    }
    */

    if (!ok) {
        self->log->log(0, "Certificate Verification: Error (%d): %s", errnum, X509_verify_cert_error_string(errnum));
    }

    if (errdepth > self->verifyClientDepth) {
        self->log->log(0, "Certificate Verification: Certificate chain too long (chain has %d certificates, but maximum allowed are only %d)", errdepth, self->verifyClientDepth);
        ok = 0;
    }

    return (ok);
}


ProbedSSL::ProbedSSL():
    lock_mutexes(NULL),
    verifyClient(TLS_CLIENT_VERIFY_NONE), verifyClientDepth(9)
{
    self = this;
    log = __server->getLog();
};

ProbedSSL::~ProbedSSL()
{
    free(lock_mutexes);
}

STACK_OF(X509_NAME) * ProbedSSL::loadCAList(const char *CAfile, const char *CApath)
{
    STACK_OF(X509_NAME) *skCAList, *sk;
    DIR *dir;
    struct dirent *direntry;
    char *cp = NULL;
    char buf[256];

    skCAList = sk_X509_NAME_new(m_x509_cmp);

    // Process CA certificate bundle file
    if (CAfile != NULL) {
        sk = SSL_load_client_CA_file(CAfile);
        for (int n = 0; sk != NULL && n < sk_X509_NAME_num(sk); n++) {
            X509_NAME *name = sk_X509_NAME_value(sk, n);
            if (sk_X509_NAME_find(skCAList, name) < 0){
                sk_X509_NAME_push(skCAList, name);
                log->log(6, "CA certificate: %s", X509_NAME_oneline(name, buf, sizeof(buf)));
            } else {
                X509_NAME_free(name);
            }
        }
        sk_X509_NAME_free(sk);
    }

    // Process CA certificate path files
    if (CApath != NULL) {
        dir = opendir(CApath);
        if(!dir){
            log->log(0, "opendir('%s') failed: %s", strerror(errno));
            return NULL;
        }
        while ((direntry = readdir(dir)) != NULL) {
            cp = (char*)alloca(strlen(CApath)+strlen(direntry->d_name)+2);
            sprintf(cp, "%s/%s", CApath, direntry->d_name);
            sk = SSL_load_client_CA_file(cp);
            for (int n = 0; sk != NULL && n < sk_X509_NAME_num(sk); n++) {
                X509_NAME *name = sk_X509_NAME_value(sk, n);
                if (sk_X509_NAME_find(skCAList, name) < 0){
                    sk_X509_NAME_push(skCAList, name);
                    log->log(2, "CA certificate: %s", X509_NAME_oneline(name, buf, sizeof(buf)));
                } else {
                    X509_NAME_free(name);
                }
            }
            sk_X509_NAME_free(sk);
        }
        closedir(dir);
    }

    sk_X509_NAME_set_cmp_func(skCAList, NULL);

    return skCAList;
}

int ProbedSSL::initialize()
{
    const DOTCONFDocumentNode * pnode = __server->getConfig()->findNode("SSLConf");
    const DOTCONFDocumentNode * node = NULL;
    const char * value = NULL;

    if(!pnode){
        log->log(0, "No SSL configuraton section found (<SSLConf>)");
        return -1;
    }

    SSL_METHOD *method;

    log->log(0, "-- Initializing OpenSSL library (%s) --", SSLeay_version(SSLEAY_VERSION));
    SSL_load_error_strings();
    SSL_library_init();

    log->log(0, "Seeding PRNG from /dev/urandom");
    RAND_load_file("/dev/urandom", 1024);

    method = SSLv23_server_method();

    log->log(0, "Initializing SSL context");
    ctx = SSL_CTX_new(method);
    if(ctx == NULL){
        unsigned long e = ERR_get_error();
        log->log(0, ERR_error_string(e, NULL));
        return -1;
    }

    log->log(4, "Initializing %d library lock mutexes", CRYPTO_num_locks());
    lock_mutexes = (pthread_mutex_t*)malloc(CRYPTO_num_locks()*sizeof(pthread_mutex_t));
    if(lock_mutexes == NULL)
        return -1;
    for(int i = 0; i<CRYPTO_num_locks(); i++){
        pthread_mutex_init(&lock_mutexes[i], NULL);
    }

    log->log(4, "Setting up threads callbacks");
    CRYPTO_set_locking_callback((T_OPENSSL_LOCKING_CALLBACK_FUNC)ProbedSSL::locking_function);
    CRYPTO_set_id_callback((T_OPENSSL_ID_CALLBACK_FUNC)ProbedSSL::id_function);

    // -- set cipher list
    node = __server->getConfig()->findNode("SSLCipherSuite", pnode);
    if(node){
        value = node->getValue();
        if(!value){
            log->log(0, "file %s, line %d: parameter %s has no value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber(), node->getName());
            return -1;
        }

        log->log(0, "Setting cipher list to '%s'", value);
        if (SSL_CTX_set_cipher_list(ctx, value) != 1){
            char * errstr = ERR_error_string(ERR_get_error(), NULL);
            log->log(0, "Failed to set context cipher list: %s", errstr);
            return -1;
        }
    }


    // -- load certificate
    node = __server->getConfig()->findNode("SSLCertificateFile", pnode);
    if(node == NULL){
        log->log(0, "mod_smtp_tls.so: SSLCertificateFile not defined");
        return -1;
    }
    value = node->getValue();
    if(!value){
        log->log(0, "file %s, line %d: parameter %s has no value",
            node->getConfigurationFileName(), node->getConfigurationLineNumber(), node->getName());
        return -1;
    }
    log->log(1, "Loading certificate from file '%s'", value);
    if(SSL_CTX_use_certificate_chain_file(ctx, value) != 1){
        char * errstr = ERR_error_string(ERR_get_error(), NULL);
        log->log(0, "Failed to load certificate: %s", errstr);
        return -1;
    }

    // -- check SSLVerifyClient configuration option
    node = __server->getConfig()->findNode("SSLVerifyClient", pnode);
    if(node){
        const char * v = node->getValue();
        if(!v){
            log->log(0, "file %s, line %d: parameter %s has no value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber(), node->getName());
            return -1;
        }
        if(!strcasecmp(v, "optional")){
            verifyClient = TLS_CLIENT_VERIFY_OPTIONAL;
        } else if(!strcasecmp(v, "require")){
            verifyClient = TLS_CLIENT_VERIFY_REQUIRE;
        } else if(strcasecmp(v, "none")){
            log->log(0, "file %s, line %d: unrecognized SSLVerifyClient value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
    }


    // -- load CA verify locations
    const char * CACertificatePath = NULL;
    const char * CACertificateFile = NULL;
    node = __server->getConfig()->findNode("SSLCACertificatePath", pnode);
    if(node){
        CACertificatePath = node->getValue();
        if(!CACertificatePath){
            log->log(0, "file %s, line %d: parameter %s has no value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber(), node->getName());
            return -1;
        }
    }
    node = __server->getConfig()->findNode("SSLCACertificateFile", pnode);
    if(node){
        CACertificateFile = node->getValue();
        if(!CACertificateFile){
            log->log(0, "file %s, line %d: parameter %s has no value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber(), node->getName());
            return -1;
        }
    }
    if(CACertificateFile || CACertificatePath){
        log->log(0, "Loading verify locations");
        if(!SSL_CTX_load_verify_locations(ctx, CACertificateFile, CACertificatePath)){
            char * errstr = ERR_error_string(ERR_get_error(), NULL);
            log->log(0, "failed to load verify locations: %s", errstr);
            return -1;
        }
        log->log(0, "setting CA list for client verification");
        STACK_OF(X509_NAME) * CAList = loadCAList(CACertificateFile, CACertificatePath);
        if(sk_X509_NAME_num(CAList) == 0 && verifyClient != TLS_CLIENT_VERIFY_NONE){
            log->log(0, "No CA certificates available for client authentication");
            return -1;
        }
        if(CAList == NULL){
            return -1;
        }
        SSL_CTX_set_client_CA_list(ctx, CAList);
    }

    // -- check SSLVerifyDepth configuration option
    node = __server->getConfig()->findNode("SSLVerifyDepth", pnode);
    if(node){
        const char * v = node->getValue();
        if(!v){
            log->log(0, "file %s, line %d: parameter %s has no value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber(), node->getName());
            return -1;
        }
        verifyClientDepth = atoi(v);
        if(verifyClientDepth <= 0 || verifyClientDepth >= 20){
            log->log(0, "file %s, line %d: Incorrect SSLVerifyDepth value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
    }


    // -- load private key
    node = __server->getConfig()->findNode("SSLCertificateKeyFile", pnode);
    if(node != NULL){
        value = node->getValue();
        if(!value){
            log->log(0, "file %s, line %d: parameter %s has no value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber(), node->getName());
            return -1;
        }

        log->log(1, "Loading private key from file '%s'", value);
        if (SSL_CTX_use_PrivateKey_file(ctx, value, SSL_FILETYPE_PEM) != 1){
            char * errstr = ERR_error_string(ERR_get_error(), NULL);
            log->log(0, "Failed to load private key: %s", errstr);
            return -1;
        }
    }


    // -- check
    if(SSL_CTX_check_private_key(ctx) != 1){
        char * errstr = ERR_error_string(ERR_get_error(), NULL);
        log->log(0, "SSL_CTX_check_private_key() failed: %s", errstr);
        return -1;
    }

    int flags = SSL_VERIFY_NONE;

    if(verifyClient == TLS_CLIENT_VERIFY_REQUIRE){
        flags = SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    } else if(verifyClient == TLS_CLIENT_VERIFY_OPTIONAL){
        flags = SSL_VERIFY_PEER;
    }

    SSL_CTX_set_verify(ctx, flags, cb_SSLVerify);
    SSL_CTX_set_verify_depth(ctx, verifyClientDepth);

    // -- set session id context for session reusage
    if(!SSL_CTX_set_session_id_context(ctx, (const unsigned char*)"probed "PACKAGE_VERSION, strlen("probed "PACKAGE_VERSION))){
        char * errstr = ERR_error_string(ERR_get_error(), NULL);
        log->log(0, "SSL_CTX_set_session_id_context() failed: %s", errstr);
        return -1;
    }

    log->log(0, "-- SSL library (%s) initialized --", SSLeay_version(SSLEAY_VERSION));
    return 0;
}

void ProbedSSL::finish()
{
    CRYPTO_set_locking_callback(NULL);
    CRYPTO_set_id_callback(NULL);

    if(lock_mutexes){
        for(int i = 0; i<CRYPTO_num_locks(); i++){
            pthread_mutex_destroy(&lock_mutexes[i]);
        }
    }

    SSL_CTX_free(ctx);

    EVP_cleanup();
    ERR_free_strings();
}

/*
InSmtpExtension * ProbedSSL::getInSmtpExtension(InSmtpConnection * _connection)
{
    return new InSmtpTLSExtension(_connection, ctx);
}
*/


