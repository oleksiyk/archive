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

#ifndef OUTPOST_INSERVER_H
#define OUTPOST_INSERVER_H

#include "outpost.h"

#include "event.h"
#include "thread_queue.h"
#include "thread_manager.h"
#include "resolver.h"
#include "message.h"
#include "message_envelope.h"

namespace Outpost {


}; /* namespace Outpost */

#include "inserver_thread.h"

/*
#include "insmtp_events.h"
#include "insmtp_connection.h"
#include "insmtp_extension.h"
#include "insmtp_extension_command.h"
#include "insmtp_registry.h"
#include "insmtp_rfc2821.h"
#include "insmtp_policy.h"
#include "insmtp_command_policy.h"

#include "smtp_module.h"
#include "userdb_module.h"
*/


namespace Outpost {

class InServer : public MainProcess
{
public:
    int initialize();
    int start();

public:
    typedef ThreadManager<ThreadQueue<int>,InServerThread> ThreadManager;

private:
    int sockfd;
    
    char * configFileName;
    static const char * requiredConfigOptions[];
    
    ThreadQueue<int> * queue;
    ThreadManager * threadManager;        
    
private:    

    int restart();
    int initsocket();

public:
    InServer(char * _configFileName);
    ~InServer();

};

}; /* namespace Outpost */

#endif
