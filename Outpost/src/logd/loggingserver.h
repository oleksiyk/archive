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

#ifndef OUTPOST_LOGSERVER_H
#define OUTPOST_LOGSERVER_H

#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include "op_sys.h"
#include "op_signal.h"
#include "logdlog.h"
#include "outpost.h"
#include "logstore_module.h"

#include "asyncresolv/mempool.h"

namespace Outpost {

class LoggingServer : public MainProcess
{
private:
    AsyncDNSMemPool * mempool; //pool is used only by store modules and LogdLog
    int sockfd;
    struct Log {
        LogStoreModule::LogStoreBase ** stores;
        size_t storesCount;
        Log():stores(NULL), storesCount(0){};
        ~Log();
    };
    struct PIDIdentity {
        pid_t pid;
        char * identity;
        PIDIdentity(pid_t _pid, char * _identity):pid(_pid), identity(strdup(_identity)){};
        ~PIDIdentity(){ free(identity); };
    };
    char * configFileName;
    static const char * requiredConfigOptions[];
    Log ** logs;
    size_t logsCount;
    PIDIdentity ** identities;
    size_t identitiesCount;

    void cleanup();
    int getLogLevel();
    int loadLogs();
    int initsocket();
    
    inline void addIdentity(pid_t pid, char * identity);
    inline const char * getIdentity(pid_t pid) const;
    
    int restart();
    
public:
    LoggingServer(char * _configFileName);
    ~LoggingServer();

    int initialize();
    int start();
};

}; /* namespace Outpost */

#endif
