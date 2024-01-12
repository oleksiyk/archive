/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#ifndef DAC_LOG_H
#define DAC_LOG_H

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
