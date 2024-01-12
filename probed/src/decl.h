/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_DECL_H
#define PROBED_DECL_H

#define DEFAULT_WAIT 300

#define DEFAULT_SESSION_TIMEOUT 15

#define DEFAULT_LOGFILE "/var/log/probed"

#define DEFAULT_USER "nobody"

#define DEFAULT_CONFIGFILE "/etc/probed.conf"

#define DEFAULT_PACKETS 10

#define ICMP_BUFSIZE 60*1024

class Log;
class Probed;
class ProbedThreadManager;
class ProbedThread;
class ProbedConfig;
class ProbedSSL;
class ProbedSSLTransport;
class ProbedCommands;
class ProbedPingThread;
class ProbedHistory;
struct ProbedPingTask;

#include "probed_sys.h"
#include "probed_signal.h"
#include "probed_string.h"

#include "thread_queue.h"
#include "probed_ping_task.h"
#include "log.h"
#include "history.h"
#include "probed_config.h"
#include "probed_thread_manager.h"
#include "probed_thread.h"
#include "probed_commands.h"
#include "probed_ping_thread.h"
#include "probed.h"
#include "probed_ssl.h"
#include "probed_ssl_transport.h"


extern const Probed * __server;

extern int _cmdDebugLevel;
extern char * _cmdUser;
extern char * _cmdHistory;
extern char * _cmdLog;
extern char * _cmdQuery;
extern int _cmdForeground;
extern int _cmdPort;
extern int _cmdTCP;

#endif

