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

#include "logdlog.h"

using namespace Outpost;

LogdLog::LogdLog(pid_t _pid, u_int8_t _loggingLevel, AsyncDNSMemPool * _mempool):
    LogBase(_pid, _loggingLevel),
    mempool(_mempool), fp(stderr)
{
}

LogdLog::~LogdLog()
{
    cleanup();
}

void LogdLog::cleanup()
{
    if(fp != stderr){
        (void) fclose(fp);
    }
    fp = stderr;
}

int LogdLog::initialize(const Configuration * config)
{
    const DOTCONFDocumentNode * node = config->findNode("LogFile");
    const char * fileName = node->getValue();
    
    if(fileName == NULL){
        error(0,0, "file %s, line %d: parameter 'LogFile' is empty",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }
    
    FILE * newfp;

    newfp = fopen(fileName, "a+");

    if(newfp == NULL){
        error(0,0, "LogdLog: fopen('%s') failed: %s", fileName, strerror(errno));
        return -1;
    }

    if(fp != stderr){
        (void) fclose(fp);
    }

    fp = newfp;

    setlinebuf(fp);

    return this->LogBase::initialize();
}

int LogdLog::logv(LogMessage::Type type, u_int8_t logid, const char * msg, va_list ap)const
{
    time_t t = time(0);

    int len = 60+strlen(msg)+strlen(LogMessage::typeNames[type]);
    char * str = (char*)mempool->alloc(len); //NOTE!! NEVER call mempool->free() from LogdLog!!!

    int f = strftime(str, len, "[%a %e %b %Y %T]", localtime(&t));

    f+=snprintf(str+f, len-f, " [outpost-logd] [%s] %s\n", LogMessage::typeNames[type], msg);
    str[f] = 0;

    vfprintf(fp, str, ap);

    return 0;
}

