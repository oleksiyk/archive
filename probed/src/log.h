/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_LOG_H
#define PROBED_LOG_H

#include "decl.h"

class Log
{
private:
    u_int8_t loggingLevel;
    FILE * fp;

public:
    Log(u_int8_t _loggingLevel);
    virtual ~Log();

    int vlog(const char * msg, va_list ap)const;
    int log(u_int8_t level, const char * msg, ...)const;

    void setLoggingLevel(int _loggingLevel){ loggingLevel = _loggingLevel; }

    int initialize(const char * fileName);
};

#endif
