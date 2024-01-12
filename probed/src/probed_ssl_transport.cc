/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#include "probed_ssl_transport.h"

ProbedSSLTransport::ProbedSSLTransport(ProbedThread * _thread, int _timeout):
    ssl(NULL), thread(_thread),
    timeout(_timeout)
{
    log = __server->getLog();
    ctx = __server->getSSL_CTX();
    fdset = thread->getfdset();
};

ProbedSSLTransport::~ProbedSSLTransport()
{
}

int ProbedSSLTransport::initialize()
{
    ssl = SSL_new(ctx);

    if(ssl == NULL){
        char errstr[120];
        ERR_error_string_n(ERR_get_error(), errstr, 120);
        log->log(1, "[%s] Failed to create new SSL context: %s", thread->getPeerName(), errstr);
        return -1;
    }

    if(SSL_set_fd(ssl, fdset->fd) != 1){
        char errstr[120];
        ERR_error_string_n(ERR_get_error(), errstr, 120);
        log->log(1, "[%s] SSL_set_fd failed: %s", thread->getPeerName(), errstr);
        cleanup();
        return -1;
    }

    fdset->events = POLLIN;

    while(1){
        int i = poll(fdset, 1, timeout * 1000);
        if(i == -1){
            if(errno != EINTR){
                log->log(0, "poll() failed: %s", strerror(errno));
            }
            cleanup();
            return -1;
        }
        if(i>0){
            int e = SSL_accept(ssl);
            if(e == 1){
                log->log(1, "[%s] [%s] SSL handshake completed", thread->getPeerName(), SSL_get_cipher(ssl));
                printPeerCertificate();
                return 0;
            }
            if(e < 0){
                int z = SSL_get_error(ssl, e);
                if(z == SSL_ERROR_WANT_READ){
                    fdset->events = POLLIN; continue;
                } else if(z == SSL_ERROR_WANT_WRITE){
                    fdset->events = POLLOUT; continue;
                } else {
                    char errstr[120];
                    ERR_error_string_n(ERR_get_error(), errstr, 120);
                    log->log(1, "[%s] SSL_accept failed: %s", thread->getPeerName(), errstr);
                    cleanup();
                    return -1;
                }
            }
            if(e == 0){
                log->log(1, "[%s] SSL handshake was shut down", thread->getPeerName());
                cleanup();
                return -1;
            }
            log->log(1, "[%s] SSL_accept() returned unexpected value %d", thread->getPeerName(), e);
            cleanup();
            return -1;
        }
        if(i == 0){ //timeout
            log->log(1, "[%s] SSL accept() timeout", thread->getPeerName());
            cleanup();
            return -1;
        }
    }

    return 0;
}


ssize_t ProbedSSLTransport::write(const void *buf, size_t count)
{
    if(count == 0){
        return 0;
    }

    fdset->events = POLLOUT;

    while(1){
        int i = poll(fdset, 1, timeout * 1000);
        if(i == -1){
            if(errno != EINTR){
                log->log(0, "poll() failed: %s", strerror(errno));
            }
            //cleanup();
            return -1;
        }

        if(i >0){
            int e = SSL_write(ssl, buf, count);
            if(e <= 0){
                switch(SSL_get_error(ssl, e)){
                case SSL_ERROR_WANT_READ:
                    fdset->events = POLLIN; continue;
                case SSL_ERROR_WANT_WRITE:
                    fdset->events = POLLOUT; continue;
                case SSL_ERROR_SYSCALL:
                    {
                    char errstr[120];
                    ERR_error_string_n(ERR_get_error(), errstr, 120);
                    log->log(1, "[%s] SSL_write: %s", thread->getPeerName(), errstr);
                    }
                    return -1;
                case SSL_ERROR_ZERO_RETURN:
                    return 0;
                case SSL_ERROR_SSL:
                    {
                    char errstr[120];
                    ERR_error_string_n(ERR_get_error(), errstr, 120);
                    log->log(1, "[%s] SSL_write: SSL error: %s", thread->getPeerName(), errstr);
                    }
                    return -1;
                }
            }
            return e;
        }
        if(i == 0){
            log->log(1, "[%s] SSL connection timeout", thread->getPeerName());
            //cleanup();
            return -1;
        }
    }


    return 0;
}

ssize_t ProbedSSLTransport::read(void *buf, size_t count)
{
    fdset->events = POLLIN;

    while(1){
        int i = poll(fdset, 1, timeout * 1000);
        if(i == -1){
            if(errno != EINTR){
                log->log(0, "poll() failed: %s", strerror(errno));
            }
            //cleanup();
            return -1;
        }

        if(i >0){
            int e = SSL_read(ssl, buf, count);
            if( e <= 0){
                switch(SSL_get_error(ssl, e)){
                    case SSL_ERROR_WANT_READ:
                        fdset->events = POLLIN; continue;
                    case SSL_ERROR_WANT_WRITE:
                        fdset->events = POLLOUT; continue;
                    case SSL_ERROR_SYSCALL:
                        {
                        char errstr[120];
                        ERR_error_string_n(ERR_get_error(), errstr, 120);
                        log->log(1, "[%s] SSL_read: %s", thread->getPeerName(), errstr);
                        }
                        //cleanup();
                        return -1;
                    case SSL_ERROR_ZERO_RETURN: //EOF
                        return 0;
                    case SSL_ERROR_SSL:
                        {
                        char errstr[120];
                        ERR_error_string_n(ERR_get_error(), errstr, 120);
                        log->log(1, "[%s] SSL_read: SSL error: %s", thread->getPeerName(), errstr);
                        }
                        //cleanup();
                        return -1;
                }
            }
            return e;
        }
        if(i == 0){
            log->log(1, "[%s] SSL connection timeout", thread->getPeerName());
            //cleanup();
            return -1;
        }
    }

    return 0;
}

void ProbedSSLTransport::printPeerCertificate()
{
    X509 * client_cert = SSL_get_peer_certificate (ssl);

    if (client_cert != NULL) {
        char *subj = NULL, *issuer = NULL;

        subj = X509_NAME_oneline (X509_get_subject_name (client_cert), NULL, 0);
        issuer = X509_NAME_oneline (X509_get_issuer_name  (client_cert), NULL, 0);

        log->log(1, "[%s] Peer certificate: subject: %s; issuer: %s", thread->getPeerName(),
            subj!=NULL?subj:"--unknown--",
            issuer!=NULL?issuer:"--unknown--");

        free(subj); free(issuer);

        X509_free (client_cert);

    } else {
        log->log(1, "[%s] Peer does not have certificate", thread->getPeerName());
    }
}

void ProbedSSLTransport::cleanup()
{
    if(ssl){
        SSL_free(ssl);
        ssl = NULL;
    }

    close(fdset->fd); fdset->fd = -1;
}

int ProbedSSLTransport::shutdown(bool immediate)
{
    if(ssl){
        int z = 0;

        for(size_t i = 0; i<2; i++){
            z = SSL_shutdown(ssl);
            if(z != 0) break;
            /*
            if(i == 0){ //send TCP FIN
                ::shutdown(fdset->fd, SHUT_WR);
            }
            */
        }

        if(z == -1){
            int e = SSL_get_error(ssl, z);
            if(e == SSL_ERROR_WANT_READ){
                fdset->events = POLLIN;
            } else if(e == SSL_ERROR_WANT_WRITE){
                fdset->events = POLLOUT;
            } else {
                char errstr[120];
                ERR_error_string_n(ERR_get_error(), errstr, 120);
                log->log(1, "[%s] SSL_shutdown failed: %s", thread->getPeerName(), errstr); immediate = true;
            }
            if(!immediate){
                while(1){
                    int i = poll(fdset, 1, timeout * 1000);
                    if(i == -1){
                        if(errno != EINTR){
                            log->log(0, "poll() failed: %s", strerror(errno));
                        }
                        break;
                    }
                    if(i>0){
                        for(size_t i = 0; i<2; i++){
                            if((z = SSL_shutdown(ssl)))  break;
                        }
                        if(z == 1) break; // ok
                        int e = SSL_get_error(ssl, z);
                        if(e == SSL_ERROR_WANT_READ){
                            fdset->events = POLLIN; continue;
                        }
                        if(e == SSL_ERROR_WANT_WRITE){
                            fdset->events = POLLOUT; continue;
                        }
                        char errstr[120];
                        ERR_error_string_n(ERR_get_error(), errstr, 120);
                        log->log(1, "[%s] SSL_shutdown failed: %s", thread->getPeerName(), errstr);
                        break;
                    }
                    if(i == 0){
                        log->log(1, "[%s] SSL_shutdown failed: timeout", thread->getPeerName());
                        break;
                    }
                }
            }
        }

        z = SSL_get_shutdown(ssl);
        if((z & SSL_SENT_SHUTDOWN) && (z & SSL_RECEIVED_SHUTDOWN)){
            log->log(1, "[%s] SSL connection closed (clean shutdown)", thread->getPeerName());
        } else {
            log->log(1, "[%s] SSL connection closed", thread->getPeerName());
        }

        cleanup();
    }

    return 0;
}
