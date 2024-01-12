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

#include "outpost.h"

using namespace Outpost;

const char * LogMessage::typeNames[] = {"critical", "error", "warning", "notice", "info"};

LogBase::~LogBase()
{
}

int LogBase::logv_uninitialized(LogMessage::Type type, u_int8_t logid, const char * msg, va_list ap)const
{
    if(type == LogMessage::MESSAGE_LOGOPEN)
        return 0;

    size_t len = 20 + strlen(msg) + strlen(LogMessage::typeNames[type]);

    char * buf = (char*)malloc(len);
    if(buf == NULL)
        return -1;

    snprintf(buf, len, "%s: %s\n", LogMessage::typeNames[type], msg);
    buf[len-1] = 0;

    vfprintf(stderr, buf, ap);

    free(buf);

    return 0;
}

int LogBase::log(LogMessage::Type type, u_int8_t logid, u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = (this->*logvMethod)(type, logid, msg, args);

    va_end(args);
    
    return ret;
}

int LogBase::vlog(LogMessage::Type type, u_int8_t logid, u_int8_t level, const char * msg, va_list ap)const
{
    if(level > loggingLevel)
        return 0;

    int ret = (this->*logvMethod)(type, logid, msg, ap);

    return ret;
}

int LogBase::error(u_int8_t logid, u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = (this->*logvMethod)(LogMessage::MESSAGE_ERROR, logid, msg, args);

    va_end(args);
    
    return ret;
}

int LogBase::critical(u_int8_t logid, u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = (this->*logvMethod)(LogMessage::MESSAGE_CRITICAL, logid, msg, args);

    va_end(args);
    
    return ret;
}

int LogBase::warning(u_int8_t logid, u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = (this->*logvMethod)(LogMessage::MESSAGE_WARNING, logid, msg, args);

    va_end(args);
    
    return ret;
}

int LogBase::notice(u_int8_t logid, u_int8_t level, const char * msg,  ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = (this->*logvMethod)(LogMessage::MESSAGE_NOTICE, logid, msg, args);

    va_end(args);
    
    return ret;
}

int LogBase::info(u_int8_t logid, u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = (this->*logvMethod)(LogMessage::MESSAGE_INFO, logid, msg, args);

    va_end(args);
    
    return ret;
}

Log::Log(pid_t _pid, u_int8_t _loggingLevel):
    LogBase(_pid, _loggingLevel),
    errlogid(0),
    concreteLogs(NULL), concreteLogsCount(0),
    socketfd(-1), logSuffix(NULL)
{
};

Log::~Log()
{
    //pthread_mutex_destroy(&logLock);
    cleanup();
}

void Log::cleanup()
{
    if(socketfd != -1)
        close(socketfd);

    socketfd = -1;
    
    errlogid = 0;

    for(size_t i = 0; i<concreteLogsCount; i++){
        delete concreteLogs[i];
    }
    free(concreteLogs);
    
    concreteLogsCount = 0;
    concreteLogs = NULL;

    free(logSuffix);
    
    logSuffix = NULL;
}


int Log::logv(LogMessage::Type type, u_int8_t logid, const char * msg, va_list ap)const
{
    int ret = 0;


    /*
    vsnprintf(logBuffer, 1024, msg, ap);
    logBuffer[1023] = 0;

    struct iovec iovector[2];
    struct msghdr udpmsg;

    logMessage.logid = logid;
    logMessage.type = type;
    logMessage.pid = pid;

    iovector[0].iov_base = &logMessage;
    iovector[0].iov_len = sizeof(logMessage) - 1;
    iovector[1].iov_base = logBuffer;
    iovector[1].iov_len = strlen(logBuffer);

    udpmsg.msg_name = (void*)&addr;
    udpmsg.msg_namelen = sizeof(struct sockaddr_un);
    udpmsg.msg_iov = iovector;
    udpmsg.msg_iovlen = 2;
    udpmsg.msg_control = NULL;
    udpmsg.msg_controllen = 0;
    udpmsg.msg_flags = 0;

    if(sendmsg(socketfd, &udpmsg, MSG_NOSIGNAL) == -1){
        fprintf(stderr, "Log::vlog(): sendmsg() failed: %s\n", strerror(errno));
        return -1;
    }
    */

    LogMessage logMessage;

    logMessage.logid = logid;
    logMessage.type = type;
    logMessage.pid = pid;

    vsnprintf(logMessage.message, 1024, msg, ap);
    logMessage.message[1023] = 0;
    size_t len = sizeof(LogMessage) - 1023 + strlen(logMessage.message);

    //pthread_mutex_lock(&logLock);

    if(sendto(socketfd, &logMessage, len, 0, (const struct sockaddr*)&addr, addrlen ) == -1){
        if(type != LogMessage::MESSAGE_LOGOPEN){
            logCriticalError("Log::vlog(): sendto() failed: %s\n", strerror(errno));
            logCriticalError("%s\n", logMessage.message);
        } else {
            logCriticalError("Failed to connect to outpost-logd: %s\n", strerror(errno));
        }
        ret = -1;
    }

    //pthread_mutex_unlock(&logLock);

    return ret;
}

void Log::logCriticalError(const char * fmt, ...)const
{
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);
    vsyslog(LOG_MAIL|LOG_CRIT, fmt, args);

    va_end(args);
}

int Log::initsocket(const char * socketName)
{
    /*
    if(pthread_mutex_init(&logLock, NULL) != 0){
        logCriticalError("Log::initialize(): pthread_mutex_init() failed");
        return -1;
    }
    */

    socketfd = socket(PF_UNIX, SOCK_DGRAM, 0);

    if(socketfd == -1){
        error(0,0, "Log::initialize(): socket() failed: %s", strerror(errno));
        return -1;
    }
    
    int sz = 262144/2;
    if(setsockopt(socketfd, SOL_SOCKET, SO_SNDBUF, &sz, sizeof(int)) == -1){
	error(0,0, "Log::initialize(): setsockopt() failed: %s", strerror(errno));
	return -1;
    }

    bzero((char*)&addr, sizeof(addr));
    addr.sun_family = AF_UNIX;    
    strncpy(addr.sun_path, socketName, sizeof(addr.sun_path)-1);
#ifdef __FreeBSD__
    addrlen = SUN_LEN(&addr);
    addr.sun_len = addrlen;
#else
    addrlen = sizeof(addr.sun_family) + strlen(addr.sun_path);
#endif

    /*
    if(sendto(socketfd, &socketName, 1, 0, (const struct sockaddr*)&addr, addrlen ) == -1){
        error(0,0, "Failed to connect to outpost-logd: %s\n", strerror(errno));
        return -1;
    }
    */

    return 0;
}

int Log::initialize(const DOTCONFDocument * config)
{
    cleanup();    

    bool errlogOK = false;
    bool maillogOK = false;

    char * errlogname = NULL;
    char * maillogname = NULL;

    u_int8_t id = 0;

    const DOTCONFDocumentNode * logNode = config->findNode("LogSuffix");
    if(logNode){
        const char * v = logNode->getValue();
        if(v){
            logSuffix = strdup(v);
        }
    }

    if(logSuffix){
        errlogname = (char*)alloca(strlen("errlog")+strlen(logSuffix)+2);
        sprintf(errlogname, "errlog-%s", logSuffix);
        maillogname = (char*)alloca(strlen("maillog")+strlen(logSuffix)+2);
        sprintf(maillogname, "maillog-%s", logSuffix);
    } else {
        errlogname = (char*)alloca(strlen("errlog")+1);
        strcpy(errlogname, "errlog");
        maillogname = (char*)alloca(strlen("maillog")+1);
        strcpy(maillogname, "maillog");
    }

    logNode = NULL;
    while( (logNode = config->findNode("Log", NULL, logNode)) != NULL){

        const char * name = logNode->getValue();

        if(name == NULL){
            error(0,0, "Log: file %s, line %d: Log parameter must have name value",
                logNode->getConfigurationFileName(), logNode->getConfigurationLineNumber());
            return -1;
        }

        concreteLogsCount++;
        concreteLogs = (ConcreteLog**)realloc(concreteLogs, concreteLogsCount*sizeof(ConcreteLog*));
        if(concreteLogs == NULL){
            --concreteLogsCount;
            return -1;
        }
        concreteLogs[concreteLogsCount-1] = new ConcreteLog(name, id);

        if(!strcasecmp(name, errlogname)){
            errlogid = id;
            errlogOK = true;
        }
        if(!strcasecmp(name, maillogname)){
            maillogOK = true;
        }
        id++;
    }
    if(!errlogOK){
        if(logSuffix){
            for(size_t i = 0; i<concreteLogsCount; i++){
                if(!strcasecmp(concreteLogs[i]->logname, "errlog")){
                    free(concreteLogs[i]->logname);
                    concreteLogs[i]->logname = (char*)malloc(strlen(logSuffix) + 8);
                    if(concreteLogs[i]->logname == NULL)
                        return -1;
                    sprintf(concreteLogs[i]->logname, "errlog-%s", logSuffix);
                    errlogid = concreteLogs[i]->logid;
                    errlogOK = true;
                }
            }
            if(!errlogOK)
                error(0,0, "Log: configuration must specify <Log \"errlog-%s\"> or <Log errlog>", logSuffix);
        } else {
            error(0,0, "Log: configuration must specify <Log errlog>");
        }
        if(!errlogOK)
            return -1;
    }
    if(!maillogOK){
        if(logSuffix){
            for(size_t i = 0; i<concreteLogsCount; i++){
                if(!strcasecmp(concreteLogs[i]->logname, "maillog")){
                    free(concreteLogs[i]->logname);
                    concreteLogs[i]->logname = (char*)malloc(strlen(logSuffix) + 9);
                    if(concreteLogs[i]->logname == NULL)
                        return -1;
                    sprintf(concreteLogs[i]->logname, "maillog-%s", logSuffix);
                    maillogOK = true;
                }
            }
            if(!maillogOK)
                error(0,0, "Log: configuration must specify <Log \"maillog-%s\"> or <Log maillog>", logSuffix);
        } else {
            error(0,0, "Log: configuration must specify <Log maillog>");
        }
        if(!maillogOK)
            return -1;
    }

    logNode = config->findNode("OUTPOST_HOME");
    const char * outpostHome = logNode->getValue();
    
    if(outpostHome == NULL){
        error(0,0,"file %s, line %d: OUTPOST_HOME value is empty",
            logNode->getConfigurationFileName(), logNode->getConfigurationLineNumber());
        return -1;
    }

    // outpostHome + /queue/sock/log + \0
    char * socketPath = (char*)alloca(strlen(outpostHome) + 16);
    sprintf(socketPath, "%s/queue/sock/log", outpostHome);
    
    if(strlen(socketPath) >= 108){
        error(0,0, "LoggingServerSocket value cannot be longer than 108 bytes");
        return -1;
    }

    if(access(socketPath, F_OK) == -1){
        error(0,0, "Can't access logging server socket (%s): %s", socketPath, strerror(errno));
        return -1;
    }
    
    if(initsocket(socketPath) == -1){
        return -1;
    }
    
    logNode = config->findNode("LoggingLevel");
    if(logNode != NULL){
        if(logNode->getValue() != NULL){
            setLoggingLevel(atoi(logNode->getValue()));
        }        
    }

    return this->LogBase::initialize();
}

u_int8_t Log::getLogId(const char * logname) const
{
    char * _logname = NULL;
    if(logSuffix){
        _logname = (char*)alloca(strlen(logname)+strlen(logSuffix)+2);
        sprintf(_logname, "%s-%s", logname, logSuffix);
    } else {
        _logname = (char*)logname;
    }
    for(size_t i = 0; i<concreteLogsCount; i++){
        if(!strcasecmp(concreteLogs[i]->logname, _logname)){
            return concreteLogs[i]->logid;
        }
    }
    if(logSuffix){ //log <Log logname-logSuffix> not found, try to find <Log logname>
        for(size_t i = 0; i<concreteLogsCount; i++){
            if(!strcasecmp(concreteLogs[i]->logname, logname)){
                return concreteLogs[i]->logid;
            }
        }
    }
    return errlogid;
}

int Log::openlog(const char * identity)
{
    return log(LogMessage::MESSAGE_LOGOPEN, 0, 0, identity);
}

