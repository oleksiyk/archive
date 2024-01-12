/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#ifndef DAC_DECL_H
#define DAC_DECL_H

#define DEFAULT_LOGFILE "/var/log/dac"

#define DEFAULT_USER "nobody"

#define DEFAULT_CONFIGFILE "/etc/dac.conf"

class Log;
class Dac;
class DacThread;
class DacConfig;

#include "dac_sys.h"
#include "dac_signal.h"
#include "dac_string.h"
#include "dac_thread.h"

#include "log.h"
#include "dac_config.h"
#include "dac.h"


extern const Dac * __server;

extern int _cmdDebugLevel;
extern char * _cmdUser;
extern char * _cmdServer;
extern char * _cmdLog;
extern char * _cmdQuery;
extern int _cmdForeground;
extern int _cmdPort;
extern int _cmdTCP;

#endif

