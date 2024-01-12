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

#ifndef OUTPOST_SERVER_H
#define OUTPOST_SERVER_H

#include "outpost.h"

namespace Outpost {

class MainProcess
{
protected:
    u_int8_t errlog;
    LogBase * log;
    Configuration * configuration;
    ModuleManager * moduleManager;
    pid_t srvpid;

private:
    char * pidfile;

private:    
    int checkPIDFile();
    int writePIDFile();
    int getLogLevel();
    
protected:
    MainProcess();
    virtual ~MainProcess();

    int setUserAndGroupID();
    
    int restart(char * _configFileName, const char ** _requiredConfigOptions);
    int initialize(char * _configFileName, const char ** _requiredConfigOptions, const char * _origin);

public:
    const LogBase * getLog()const{ return log;}
    const DOTCONFDocument * getConfiguration()const{ return configuration; }
};

}; /* namespace Outpost */

#endif
