/*  Copyright (C) 2003 FOSS-On-Line <http://www.foss.kharkov.ua>,
*   Aleksey Krivoshey <krivoshey@users.sourceforge.net>
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


#include "inserver.h"

#ifdef __linux__
static void null_signal_handler(int){};
#endif

using namespace Outpost;

const MainProcess * Outpost::__server = NULL;

const char * InServer::requiredConfigOptions[] = { 
    "OUTPOST_HOME",
    "Log",
    "User",
    "Group",
    "PIDFile"
    "UserDatabase",
    NULL
};

InServer::InServer(char * _configFileName):
    sockfd(-1),
    configFileName(_configFileName),
    queue(NULL), threadManager(NULL)
{
    Outpost::__server = this;
}

InServer::~InServer()
{
    if(sockfd != -1){
        (void)close(sockfd);
    }

    delete threadManager;
    delete queue;

    log->notice(errlog, 0, "outpost-inserverd exited");
}

int InServer::initialize()
{
    outpost_pthread_block_signal(SIGIO);

    if(this->MainProcess::initialize(configFileName, requiredConfigOptions, "inserverd") == -1){
        return -1;
    }

    if(initsocket() == -1){
        return -1;
    }

    if(setUserAndGroupID() == -1){
        return -1;
    }

    threadManager = new ThreadManager(2, INSMTP_MAX_THREADS);

    queue = new ThreadQueue<int>(threadManager, INSMTP_HIGHWATERMARK, INSMTP_FD_QUEUE_SIZE);
    
    if(queue->initialize() == -1){
        log->error(errlog, 0, "failed to initialize thread_queue");
        return -1;
    }

    threadManager->initialize(queue);

    delete configuration; configuration = NULL;

    log->notice(errlog, 0, "outpost-insmtpd started");

    return 0;
}

int InServer::initsocket()
{
    struct sockaddr_in addr;

    addr.sin_port = htons(25);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    const DOTCONFDocumentNode * node = configuration->findNode("Port");
    if(node){
        const char * v = node->getValue();
        if(!v){
            log->error(errlog, 0, "file %s, line %d: Port parameter is empty",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
        int port = atoi(v);
        if(port < 1 || port > 65535){
            log->error(errlog, 0, "file %s, line %d: Invalid 'Port' value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
        addr.sin_port = htons(port);
    }

    node = configuration->findNode("BindAddress");
    if(node){
        const char * v = node->getValue();

        if(!v){
            log->error(errlog, 0, "file %s, line %d: BindAddress parameter is empty",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }

        if(strcmp(v, "*")){

            struct hostent * hep = gethostbyname(v);

            if ((!hep) || (hep->h_addrtype != AF_INET || !hep->h_addr_list[0])) {
                log->error(errlog, 0, "file %s, line %d: Cannot resolve hostname '%s'",
                    node->getConfigurationFileName(), node->getConfigurationLineNumber(), v);
                return -1;
            }

            if (hep->h_addr_list[1]) {
                log->error(errlog, 0, "file %s, line %d: Host %s has multiple addresses, you must choose one explicitly for use",
                    node->getConfigurationFileName(), node->getConfigurationLineNumber(), v);
                return -1;
            }

            addr.sin_addr.s_addr = ((struct in_addr *) (hep->h_addr))->s_addr;
        }
    }

    sockfd = socket(PF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        log->error(errlog,0, "socket failed: %s", strerror(errno));
        return -1;
    }

    if(fcntl(sockfd, F_SETFL, O_NONBLOCK|O_ASYNC) == -1){ //NOTE: O_ASYNC: AIX support only ioctl(sockefd, FIOASYNC, &arg)
        log->error(errlog,0,"setting O_NONBLOCK|O_ASYNC failed: %s", strerror(errno));
        return -1;
    }
    if(fcntl(sockfd, F_SETOWN, getpid()) == -1){
        log->error(errlog,0, "fcntl(F_SETOWN) failed: %s", strerror(errno));
        return -1;
    }

    int arg = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(char*)&arg,sizeof(arg)) == -1){
        log->error(errlog,0, "setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
        return -1;
    }
    
    if(bind(sockfd, (sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
        log->error(errlog,0, "bind() failed: %s", strerror(errno));
        return -1;
    }    
    if(listen(sockfd, 511) == -1){
        log->error(errlog,0, "listen failed:", strerror(errno));
        return -1;
    }

    return 0;
}

int InServer::restart()
{
    outpost_block_signal(SIGIO);

    threadManager->stopThreads();

    delete queue; queue = NULL;
    delete threadManager; threadManager = NULL;

    if(this->MainProcess::restart(configFileName, requiredConfigOptions) == -1){
        return -1;
    }

    threadManager = new ThreadManager(2, INSMTP_MAX_THREADS);

    queue = new ThreadQueue<int>(threadManager, INSMTP_HIGHWATERMARK, INSMTP_FD_QUEUE_SIZE);
    
    if(queue->initialize() == -1){
        log->error(errlog, 0, "failed to initialize thread_queue");
        return -1;
    }

    threadManager->initialize(queue);

    if(threadManager->startThreads() == -1){
        log->notice(errlog, 0, "failed to start threads. shutting down");
        return -1;
    }

    delete configuration; configuration = NULL;

    log->info(errlog, 0, "outpost-insmtpd restarted");

    return 0;
}

int InServer::start()
{
    int clfd, signum;
    struct sockaddr_in claddr;
    size_t addrlen = sizeof(claddr);
    sigset_t sa_mask;

#ifdef __linux__
    outpost_signal(SIGINT, null_signal_handler);
    outpost_signal(SIGTERM, null_signal_handler);
    outpost_signal(SIGHUP, null_signal_handler);
    outpost_signal(SIGIO, null_signal_handler);
#endif
    outpost_signal(SIGPIPE, SIG_IGN);

    if(threadManager->startThreads() == -1){
        log->notice(errlog, 0, "failed to start threads. shutting down");
        return -1;
    }

    outpost_pthread_block_signal(SIGINT);
    outpost_pthread_block_signal(SIGTERM);
    outpost_pthread_block_signal(SIGHUP);
    outpost_pthread_block_signal(SIGIO);

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, SIGTERM);
    sigaddset(&sa_mask, SIGINT);
    sigaddset(&sa_mask, SIGHUP);
    sigaddset(&sa_mask, SIGIO);

    for(;;){
        sigwait(&sa_mask, &signum);
        //log->info(errlog, 0, "ACTIVE CONNECTIONS: %d", activeconnections);
        if( signum == SIGIO ){
            while( (clfd = accept(sockfd, (sockaddr*)&claddr, &addrlen)) > 0 ){
                queue->push(clfd);
            }
            if( errno != EAGAIN){
                if(errno == EMFILE || errno == ENFILE ){
                    log->warning(errlog, 0, "RUNNING OUT OF MAXIMUM OPEN FILE DESCRIPTORS. NOT ACCEPTING NEW CONNECTIONS FOR 20 sec");
                    outpost_sleep(20, 0);
                } else {
                    log->error(errlog, 0, "accept() failed: %s", strerror(errno));
                    break;
                }
            }
            continue;
        }
        if( signum == SIGTERM || signum == SIGINT){
            break;
        }
        if( signum == SIGHUP ){
            log->notice(errlog, 0, "outpost-inserverd restarting");
            if(restart() == -1){
                break;
            }
        }
    }

    log->notice(errlog, 0, "outpost-inserverd shutting down");

    if(threadManager)
        threadManager->stopThreads();

    return 0;
}

int main(int argc, char *argv[])
{
    /* enable core dump */
    struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);

    if(argc < 2){
        fprintf(stderr, "Usage: %s config.conf\n", argv[0]);
        return 1;
    }

    umask(0037);

    if(access(argv[1], R_OK) == -1){
        fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1], strerror(errno));
        return 1;
    }

    tzset();

    InServer * server = new InServer(argv[1]);

    int ret = server->initialize();

    if(ret != -1){
        ret = server->start();
    }

    delete server;

    return -ret;
}
