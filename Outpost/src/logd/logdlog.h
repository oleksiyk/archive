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

#ifndef OUTPOST_LOGDLOG_H
#define OUTPOST_LOGDLOG_H

#include "op_sys.h"
#include "outpost.h"
#include "asyncresolv/mempool.h"

namespace Outpost {

/*
* LogdLog is used only in outpost-logd process.
*
* if we were using in outpost-logd class Log that would be enormous and difficult to avoid problems
* on start/stop/restart
*/

class LogdLog : public LogBase
{
private:
    AsyncDNSMemPool * mempool; //NOTE! we use mempool just to get some memory for storing message, but NEVER
                      // call mempool->free() from within this class! This is because this mempool
                      // is used by log store modules, and they can allocate some memory with it and
                      // then log message through this class
                      // I know, I must have been create own mempool for logdlog, but I wanted to save memory =)
    FILE * fp;
    virtual int logv(LogMessage::Type type, u_int8_t logid, const char * msg, va_list ap)const;
    
    void cleanup();

public:
    LogdLog(pid_t _pid, u_int8_t _loggingLevel, AsyncDNSMemPool * _mempool);
    virtual ~LogdLog();

    virtual int initialize(const Configuration * config);

    virtual u_int8_t getLogId(const char * logname) const { return 0; }
};

}; /* namespace Outpost */

#endif
