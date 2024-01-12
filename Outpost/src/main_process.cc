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


#include "main_process.h"

using namespace Outpost;

MainProcess::MainProcess():
    errlog(0), log(NULL),
    configuration(NULL), userdb(NULL),
    moduleManager(NULL),
    pidfile(NULL), messageQueueHelper(NULL)
{
    log = new Log(getpid(), 2);
}

MainProcess::~MainProcess()
{
    delete messageQueueHelper;
    delete configuration;
    delete userdb;
    delete moduleManager;

    if(pidfile){
        if(unlink(pidfile) == -1){
            log->error(errlog, 0, "unlink(%s) failed %s", pidfile, strerror(errno));
        }
        free(pidfile);
    }

    delete log;
}

int MainProcess::checkPIDFile()
{
    const DOTCONFDocumentNode * node = configuration->findNode("PIDFile");

    pidfile = strdup(node->getValue());

    if(pidfile == NULL){
        log->error(errlog, 0, "file %s, line %d: parameter PIDFile must have value",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

    if(access(pidfile, F_OK) == 0){ //file already exist
        char pidbuf[10];
        int fd = open(pidfile, O_RDWR);
        if(fd == -1){
            log->error(errlog,0, "open(%s) failed: %s", pidfile, strerror(errno));
            return -1;
        }
        int sz = read(fd, pidbuf, 10);
        if(sz == -1){
            log->error(errlog,0, "read(%s) failed: %s", pidfile, strerror(errno));
            close(fd);
            return -1;
        }
        pid_t pid = 0;
        if(sz > 2){
            pidbuf[sz-1] = 0;
            pid = atoi(pidbuf);
            if(kill(pid, 0) == -1){
                if(errno == ESRCH){
                    log->warning(errlog,0, "unclean shutdown of previous run");
                } else {
                    log->error(errlog,0, "Failed to ping process with id %d: %s", pid, strerror(errno));
                    close(fd);
                    return -1;
                }
            } else {
                log->error(errlog,0, "already running, pid %d", pid);
                close(fd);
                return -1;
            }
        }
        close(fd);
    }

    return 0;
}

int MainProcess::writePIDFile()
{
    int fd = open(pidfile, O_RDWR|O_CREAT|O_TRUNC, 00644);

    if(fd == -1){
        log->error(errlog, 0, "Failed to open PIDFile %s: %s", pidfile, strerror(errno));
        return -1;
    }

    pid_t pid = getpid();
    char pidbuf[32];

    snprintf(pidbuf, 32, "%ld\n", (long) pid); pidbuf[31] = 0;
    write(fd, pidbuf, strlen(pidbuf));

    close(fd);

    return 0;
}

int MainProcess::setUserAndGroupID()
{
    uid_t uid;
    gid_t gid;

    const DOTCONFDocumentNode * node = configuration->findNode("User");

    const char * user = node->getValue();
    if(user != NULL){
        const struct passwd * pwd = getpwnam(user);
        if(pwd != NULL){
            uid = pwd->pw_uid;
        } else {
            log->error(errlog, 0, "No such user: '%s'", user);
            return -1;
        }
    } else {
        log->error(errlog, 0, "'User' parameter is empty");
        return -1;
    }

    node = configuration->findNode("Group");
    const char * group = node->getValue();
    if(group != NULL){
        const struct group * pwd = getgrnam(group);
        if(pwd != NULL){
            gid = pwd->gr_gid;
        } else {
            log->error(errlog, 0, "No such group: '%s'", group);
            return -1;
        }
    } else {
        log->error(errlog, 0, "'Group' parameter is empty");
        return -1;
    }

    if(setregid(gid, gid) == -1) {
        log->error(errlog, 0, "Failed to change group id to '%s': %s", group, strerror(errno));
        return -1;
    }
    if(setreuid(uid, uid) == -1) {
        log->error(errlog, 0, "Failed to change user id to '%s': %s", user, strerror(errno));
        return -1;
    }

    return 0;
}

int MainProcess::getLogLevel()
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

int MainProcess::initialize(char * _configFileName, const char ** _requiredConfigOptions, const char * _origin, bool initUserDB)
{
    configuration = new Configuration(DOTCONFDocument::CASESENSETIVE);
    configuration->setRequiredOptionNames(_requiredConfigOptions);
    
    if(configuration->setContent(_configFileName) == -1){
        return -1;
    }

    if(checkPIDFile() == -1){
        free(pidfile); pidfile = NULL;
        return -1;
    }
    
    if(log->initialize(configuration) == -1){
        free(pidfile); pidfile = NULL;
        return -1;
    }
    
    log->setLoggingLevel(getLogLevel());

    const char * s = configuration->findNode("OUTPOST_HOME")->getValue();
    if(!s){
        log->error(errlog, 0, "'OUTPOST_HOME' parameter is empty");
        free(pidfile); pidfile = NULL;
        return -1;
    }

    if(access(s, R_OK|W_OK|X_OK) == -1){
        log->error(errlog, 0, "access(%s): %s", s, strerror(errno));
        free(pidfile); pidfile = NULL;
        return -1;
    }

    if(setenv("OUTPOST_HOME", s, 1) == -1){
        log->error(errlog, 0, "setenv(): %s", strerror(errno));
        free(pidfile); pidfile = NULL;
        return -1;
    }

    if(outpost_detach(NULL) == -1){
        free(pidfile); pidfile = NULL;
        return -1;
    }
    
    srvpid = getpid();
    log->setLoggingPID(srvpid);

    if(writePIDFile() == -1){
        free(pidfile); pidfile = NULL;
        return -1;
    }

    if(log->openlog(_origin) == -1){
        return -1;
    }

    errlog = log->getLogId("errlog");

    messageQueueHelper = new OutpostMessageQueueHelper(log);
    if(messageQueueHelper->initialize() == -1){
        return -1;
    }

    moduleManager = new ModuleManager();

    if(initUserDB){
        log->info(errlog, 0, "setting up user database");

        const DOTCONFDocumentNode * node = configuration->findNode("UserDatabase");
        userdb = new UserDB();
        if(userdb->initialize(node) == -1){
            return -1;
        }
    }

    return 0;
}

int MainProcess::restart(char * _configFileName, const char ** _requiredConfigOptions, bool initUserDB)
{
    delete configuration; configuration = NULL;
    delete userdb; userdb = NULL;
    delete moduleManager; moduleManager = NULL;

    configuration = new Configuration(DOTCONFDocument::CASESENSETIVE);
    configuration->setRequiredOptionNames(_requiredConfigOptions);

    if(configuration->setContent(_configFileName) == -1){
        return -1;
    }

    const char * s = configuration->findNode("OUTPOST_HOME")->getValue();
    if(!s){        
        log->error(errlog, 0, "'OUTPOST_HOME' parameter is empty");
        return -1;
    }
    
    if(log->initialize(configuration) == -1){
        return -1;
    }
    
    log->setLoggingLevel(getLogLevel());

    /*
    Log * newlog = new Log(getpid(), 2);
    
    if(newlog->initialize(configuration) == -1){
        return -1;
    }
    
    log->setLoggingLevel(getLogLevel());

    delete log; log = newlog;
    */

    errlog = log->getLogId("errlog");

    moduleManager = new ModuleManager();

    if(initUserDB){
        log->info(errlog, 0, "setting up user_db backend");

        userdb = new UserDB();
        const DOTCONFDocumentNode * node = configuration->findNode("UserDatabase");
        if(userdb->initialize(node) == -1){
            return -1;
        }
    }

    return 0;
}
