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

#include "loggingserver.h"

using namespace Outpost;

const MainProcess * Outpost::__server = NULL;

#ifdef __linux__
static void null_signal_handler(int){};
#endif

const char * LoggingServer::requiredConfigOptions[] = {
    "OUTPOST_HOME",
    "Log",
    "LoggingServerModules",
    "LogFile",
    "PIDFile",
    NULL
};

LoggingServer::Log::~Log()
{
    for(size_t i = 0; i<storesCount; i++){
        delete stores[i];
    }
    free(stores);
}

LoggingServer::LoggingServer(char * _configFileName):
    mempool(NULL),
    sockfd(-1), configFileName(_configFileName),
    logs(NULL), logsCount(0),
    identities(NULL), identitiesCount(0)
{
    Outpost::__server = this;
}

LoggingServer::~LoggingServer()
{
    if(sockfd != -1){
        (void)close(sockfd);
    }
    
    for(size_t i = 0; i<identitiesCount; i++){
        delete identities[i];
    }
    free(identities); identities = NULL; identitiesCount = 0;

    cleanup();

    log->notice(0,0, "outpost-logd exited");

    Outpost::__server = NULL;

    delete mempool;
}

void LoggingServer::cleanup()
{
    mempool->free();
    for(size_t i = 0; i<logsCount; i++){
        delete logs[i];
    }
    free(logs); logs = NULL; logsCount = 0;
    /*
    for(size_t i = 0; i<identitiesCount; i++){
        delete identities[i];
    }
    free(identities); identities = NULL; identitiesCount = 0;
    */
}

int LoggingServer::getLogLevel()
{
    int logLevel = 0;

    const DOTCONFDocumentNode * node = configuration->findNode("LoggingLevel");

    if(node != NULL){
        const char * str_logLevel = node->getValue();
        if(str_logLevel != NULL){            
            logLevel = atoi(str_logLevel);
        }
    }

    return logLevel;
}

int LoggingServer::loadLogs()
{
    const DOTCONFDocumentNode * logNode = NULL;
    const DOTCONFDocumentNode * storeNode = NULL;

    /*
    bool errlogOK = false;
    bool maillogOK = false;
    */

    const char * moduleAlias = NULL;
    Module * module = NULL;
    LogStoreModule::LogStoreBase * logStore = NULL;

    while( (logNode = configuration->findNode("Log", NULL, logNode)) != NULL){

        const char * name = logNode->getValue();
        if(name == NULL){
            log->error(0, 0, "file %s, line %d: Log parameter must have name value",
                    logNode->getConfigurationFileName(), logNode->getConfigurationLineNumber());
                return -1;
        }
        /*
        if(!strcasecmp(name, "errlog")){
            errlogOK = true;
        }
        if(!strcasecmp(name, "maillog")){
            maillogOK = true;
        }
        */
        storeNode = NULL;
        logsCount++;
        logs = (Log**)realloc(logs, logsCount*sizeof(Log*));
        logs[logsCount-1] = new Log;

        while((storeNode = configuration->findNode("Store", logNode, storeNode)) != NULL){
            moduleAlias = storeNode->getValue();
            if(moduleAlias == NULL){
                log->error(0,0, "file %s, line %d: Store parameter must have module alias value",
                    storeNode->getConfigurationFileName(), storeNode->getConfigurationLineNumber());
                return -1;
            }
            module = moduleManager->getModuleByAlias(moduleAlias);
            if(module == NULL){
                log->error(0,0, "file %s, line %d: Cannot find store module for alias '%s'",
                    storeNode->getConfigurationFileName(), storeNode->getConfigurationLineNumber(), moduleAlias);
                return -1;
            }
            logStore = ((LogStoreModule*)module)->createStore(mempool);
            if(logStore == NULL){
                return -1;
            } 
            if (logStore->initialize(storeNode) == -1){
                delete logStore;
                return -1;
            }
            logs[logsCount-1]->storesCount++;
            logs[logsCount-1]->stores = 
                (LogStoreModule::LogStoreBase **)realloc(logs[logsCount-1]->stores,
                    logs[logsCount-1]->storesCount*sizeof(LogStoreModule::LogStoreBase *));

            logs[logsCount-1]->stores[logs[logsCount-1]->storesCount-1] = logStore;
            mempool->free(); //to clear memory used by logdlog
        }
        if(logs[logsCount-1]->storesCount == 0){
            log->error(0,0, "file %s, line %d: LogCategory must have at least one store defined",
                logNode->getConfigurationFileName(), logNode->getConfigurationLineNumber());
            return -1;
        }
    }
    /*
    if(!errlogOK){
        log->error(0,0, "configuration must specify <Log errlog>");
        return -1;
    }
    if(!maillogOK){
        log->error(0,0, "configuration must specify <Log maillog>");
        return -1;
    }
    */

    return 0;
}

int LoggingServer::initsocket()
{
    struct sockaddr_un addr;
    
    const DOTCONFDocumentNode * node = configuration->findNode("OUTPOST_HOME");
    const char * outpostHome = node->getValue();
    
    if(outpostHome == NULL){
        log->error(0,0,"file %s, line %d: OUTPOST_HOME value is empty",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

    // outpostHome + /queue/sock/log + \0
    char * socketPath = (char*)alloca(strlen(outpostHome) + 16);
    sprintf(socketPath, "%s/queue/sock/log", outpostHome);

    if(strlen(socketPath) >= sizeof(addr.sun_path)){
        log->error(0,0, "LoggingServerSocket value cannot be longer than %d bytes on this system", sizeof(addr.sun_path));
        return -1;
    }

    sockfd = socket(PF_UNIX, SOCK_DGRAM, 0);

    if(sockfd == -1){
        log->error(0,0, "UNIX socket() failed: %s", strerror(errno));
        return -1;
    }
    
    int sz = 262144/2;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(int)) == -1){
	log->error(0,0, "setsockopt(SO_RCVBUF) failed: %s", strerror(errno));
	return -1;
    }
    
    (void) unlink(socketPath);

    bzero((char*)&addr, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path)-1);
#ifdef __FreeBSD__
    socklen_t addrlen = SUN_LEN(&addr);
    addr.sun_len = addrlen;
#else
    socklen_t addrlen = sizeof(addr.sun_family) + strlen(addr.sun_path);
#endif

    if(fcntl(sockfd, F_SETFL, O_NONBLOCK|O_ASYNC) == -1){ //NOTE: O_ASYNC: AIX support only ioctl(sockefd, FIOASYNC, &arg)
        log->error(0,0, "setting O_NONBLOCK|O_ASYNC failed: %s", strerror(errno));
        return -1;
    }
    if(fcntl(sockfd, F_SETOWN, getpid()) == -1){
        log->error(0,0, "fcntl(F_SETOWN) failed: %s", strerror(errno));
        return -1;
    }
    
    int arg = 1;
    (void) setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(char*)&arg,sizeof(arg));

    mode_t pr_mask = umask(0007);
    
    if(bind(sockfd, (sockaddr*)&addr, addrlen) == -1){
        log->error(0,0, "UNIX bind() failed: %s", strerror(errno));
        return -1;
    }

    (void) umask(pr_mask);

    return 0;
}

void LoggingServer::addIdentity(pid_t pid, char * identity)
{
    for(size_t i = 0; i<identitiesCount; i++){
        if(!strcmp(identities[i]->identity, identity)){
            free(identities[i]->identity);
            identities[i]->identity = strdup(identity);
            identities[i]->pid = pid;
            return;
        }
    }
    identitiesCount++;
    identities = (PIDIdentity**)realloc(identities, identitiesCount*sizeof(PIDIdentity*));
    identities[identitiesCount-1] = new PIDIdentity(pid, identity);
}

const char * LoggingServer::getIdentity(pid_t pid) const
{
    for(size_t i = 0; i<identitiesCount; i++){
        if(identities[i]->pid == pid){
            return identities[i]->identity;
        }
    }
    log->notice(0, 2, "no identity record for pid %d", pid);
    return "";
}

int LoggingServer::initialize()
{
    mempool = new AsyncDNSMemPool(2048); //would this be enough for all store modules and LogdLog?
    if(mempool->initialize() == -1){
        return -1;
    }
    
    delete log;
    log = new LogdLog(getpid(), 2, mempool);
    
    //log->info(0, 2, "Log class substituted");
    
    if(this->MainProcess::initialize(configFileName, requiredConfigOptions, "logd", false) == -1){
        return -1;
    }

    if(initsocket() == -1){
        return -1;
    }
    
    const DOTCONFDocumentNode * node = configuration->findNode("LoggingServerModules");
    if( moduleManager->loadModules(node, Module::OUTPOST_LOG_STORE_MODULES) == -1){
        return -1;
    }
    
    mempool->free();

    if(loadLogs() == -1){
        return -1;
    }

    (void) outpost_block_signal(SIGTERM);
    (void) outpost_block_signal(SIGINT);
    (void) outpost_block_signal(SIGHUP);
    (void) outpost_block_signal(SIGIO);

    delete configuration; configuration = NULL;

    log->notice(0, 0, "outpost-logd started");
    
    mempool->free();

    return 0;
}

int LoggingServer::restart()
{
    cleanup();
    
    if(this->MainProcess::initialize(configFileName, requiredConfigOptions, "logd", false) == -1){
        return -1;
    }
    
    const DOTCONFDocumentNode * node = configuration->findNode("LoggingServerModules");
    if( moduleManager->loadModules(node, Module::OUTPOST_LOG_STORE_MODULES) == -1){
        return -1;
    }
    mempool->free();

    if(loadLogs() == -1){
        return -1;
    }
    
    delete configuration; configuration = NULL;

    log->info(0,0, "outpost-logd restarted");
    mempool->free();

    return 0;
}

int LoggingServer::start()
{
    char buf[sizeof(LogMessage)];
    LogMessage * logMessage = (LogMessage*)buf;

    Log * destLog = NULL;
    const char * identity = NULL;

    /*
    int errCount = 0;
    time_t firstErrorTime = 0;
    */

    ssize_t k = 0;

    int signum;
    sigset_t sa_mask;

#ifdef __linux__
    /* install dummy null signal handlers to overcome bug in early glibc 2.2.5 */
    (void) outpost_signal(SIGTERM, null_signal_handler);
    (void) outpost_signal(SIGINT, null_signal_handler);
    (void) outpost_signal(SIGHUP, null_signal_handler);
    (void) outpost_signal(SIGIO, null_signal_handler);
#endif

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, SIGTERM);
    sigaddset(&sa_mask, SIGINT);
    sigaddset(&sa_mask, SIGHUP);
    sigaddset(&sa_mask, SIGIO);

    for(;;){
        sigwait(&sa_mask, &signum);
#ifdef __FreeBSD__
	outpost_block_signal(SIGIO);
#endif
        if( signum == SIGIO ){
            while ( (k = recv(sockfd, buf, sizeof(LogMessage), 0)) > 0){
                /*
                if(k == 1){ //ping
                    continue;
                }
                */
                buf[k-1] = 0;
                if(logMessage->type == LogMessage::MESSAGE_LOGOPEN){
                    logMessage->message[24] = 0; //limit identity length
                    addIdentity(logMessage->pid, logMessage->message);
                    log->info(0, 2, "opened log for '%s' (pid %d)",
                        logMessage->message, logMessage->pid);
                    mempool->free();
                    continue;
                }
                if(logMessage->type > LogMessage::MESSAGE_INFO){
                    log->warning(0, 0, "received incorrect message type %d", logMessage->type);
                    mempool->free();
                    continue;
                }
                if(logMessage->logid >= logsCount){
                    log->warning(0, 0, "received incorrect message logid %d", logMessage->logid);
                    mempool->free();
                    continue;
                }
                destLog = logs[logMessage->logid];
                identity = getIdentity(logMessage->pid);
                for(size_t i = 0; i < destLog->storesCount; i++){
                    destLog->stores[i]->storeMessage(logMessage->type, logMessage->logid, identity, logMessage->message);
                }
            }
            if( errno != EAGAIN){
                log->error(0,0, "recv() failed: %s", strerror(errno));
                mempool->free();
                /*
                errCount++;
                if(errCount == 1){
                    firstErrorTime = time(0);
                    continue;
                }
                if(errCount > 10){
                    if((time(0) - firstErrorTime) <= 2){
                        log->error(0,0, "More than 10 network errors occured in 2 seconds, exiting");
                        break;
                    } else {
                        errCount = 0;
                    }
                }
                */
            }
            continue;
        }
        if( signum == SIGTERM || signum == SIGINT){
            break;
        }
        if( signum == SIGHUP ){
            log->notice(0, 0, "SIGHUP: restarting");
            if(restart() == -1)
                break;
        }
    }
    log->notice(0, 1, "outpost-logd resuming operations");

    return 0;
}

int main(int argc, char *argv[])
{
    /* enable core dump */
    struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);

    if( getuid() == 0 || geteuid() == 0){
        fprintf(stderr, "%s will not run as root\n", argv[0]);
        return 1;
    }

    if(argc < 2){
        fprintf(stderr, "Usage: %s path_to_log.conf\n", argv[0]);
        return 1;
    }

    umask(0037);

    if(access(argv[1], R_OK) == -1){
        fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1], strerror(errno));
        return 1;
    }

    tzset();

    LoggingServer * loggingserver = new LoggingServer(argv[1]);

    int ret = loggingserver->initialize();

    if(ret != -1){
        ret = loggingserver->start();
    }

    delete loggingserver;

    return -ret;
}
