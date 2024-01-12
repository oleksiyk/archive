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

#ifndef OUTPOST_LOG_H
#define OUTPOST_LOG_H

#include "op_sys.h"
#include "dotconf++/dotconfpp.h"

namespace Outpost {

#pragma pack(1)

struct LogMessage
{
    enum Type {
        MESSAGE_CRITICAL = 0,
        MESSAGE_ERROR = 1,
        MESSAGE_WARNING = 2,
        MESSAGE_NOTICE = 3,
        MESSAGE_INFO = 4,
        MESSAGE_LOGOPEN = 32
    };
    static const char * typeNames[];
    u_int8_t    logid;
    Type        type;
    pid_t       pid;
    char        message[1024];
};

#pragma pack()

/*
* base class for Outpost logging
*/
class LogBase
{
private:
    u_int8_t loggingLevel;
    typedef int (LogBase::*logvMethodType)(LogMessage::Type type, u_int8_t logid, const char * msg, va_list ap)const;
    logvMethodType logvMethod;

protected:
    pid_t pid;

    virtual int logv(LogMessage::Type type, u_int8_t logid, const char * msg, va_list ap)const = 0;
    virtual int logv_uninitialized(LogMessage::Type type, u_int8_t logid, const char * msg, va_list ap)const;
    //int initialize(){ return 0;};

public:
    LogBase(pid_t _pid, u_int8_t _loggingLevel):
        loggingLevel(_loggingLevel), logvMethod(&LogBase::logv_uninitialized), pid(_pid){};
    virtual ~LogBase();

    int log(LogMessage::Type type, u_int8_t logid, u_int8_t level, const char * msg, ...)const;
    int vlog(LogMessage::Type type, u_int8_t logid, u_int8_t level, const char * msg, va_list ap)const;
    int error(u_int8_t logid, u_int8_t level, const char * msg, ...)const;
    int critical(u_int8_t logid, u_int8_t level, const char * msg, ...)const;
    int warning(u_int8_t logid, u_int8_t level, const char * msg, ...)const;
    int notice(u_int8_t logid, u_int8_t level, const char * msg,  ...)const;
    int info(u_int8_t logid, u_int8_t level, const char * msg, ...)const;

    void setLoggingPID(pid_t _pid){ pid = _pid; }
    void setLoggingLevel(int _loggingLevel){ loggingLevel = _loggingLevel; }
    
    virtual int openlog(const char * identity) { return 0; }
    
    virtual int initialize(const DOTCONFDocument * config = NULL){ logvMethod = &LogBase::logv; return 0;};

    virtual u_int8_t getLogId(const char * logname) const = 0;
};


/*
*
* logging class used in all Outpost processes except for outpost-logd
* sends log message to logging server
*
*/
class Log : public LogBase
{
private:
    u_int8_t errlogid;

    struct ConcreteLog {
        char * logname;
        u_int8_t logid;
        ConcreteLog(const char * _name, u_int8_t _id):
            logname(strdup(_name)), logid(_id){};
        ~ConcreteLog(){ free(logname); };
    };
    ConcreteLog ** concreteLogs;
    size_t concreteLogsCount;

private:
    //mutable pthread_mutex_t logLock;
    struct sockaddr_un addr;
    socklen_t addrlen;
    int socketfd;

    char * logSuffix;
    
    void cleanup();

    int initsocket(const char * socketName);
    void logCriticalError(const char * fmt, ...)const;    
    virtual int logv(LogMessage::Type type, u_int8_t logid, const char * msg, va_list ap)const;

public:
    Log(pid_t _pid, u_int8_t _loggingLevel);
    virtual ~Log();

    int initialize(const DOTCONFDocument * config);
    int openlog(const char * identity);

    virtual u_int8_t getLogId(const char * logname) const;
};

}; /* namespace Outpost */

#endif
