/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#include "log.h"

Log::Log(u_int8_t _loggingLevel):
    loggingLevel(_loggingLevel), fp(stderr)
{
};

Log::~Log()
{
    if(fp != stderr){
        (void) fclose(fp);
    }
}

int Log::initialize(const char * fileName)
{
    FILE * newfp;

    newfp = fopen(fileName, "a+");

    if(newfp == NULL){
        log(0, "Log: fopen('%s') failed: %s", fileName, strerror(errno));
        return -1;
    }

    if(fp != stderr){
        (void) fclose(fp);
    }

    fp = newfp;

    setlinebuf(fp);

    return 0;
}

int Log::vlog(const char * msg, va_list ap)const
{
    time_t t = time(0);

    int len = 60+strlen(msg);
    char * str = (char*)malloc(len);

    int f = strftime(str, len, "%b %e %Y %H:%M:%S", localtime(&t));

    f+=snprintf(str+f, len-f, " %s\n", msg);
    str[f] = 0;

    vfprintf(fp, str, ap);
    if(_cmdForeground && fp != stderr){
        vfprintf(stderr, str, ap);
    }

    free(str);

    return 0;
}

int Log::log(u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = vlog(msg, args);

    va_end(args);

    return ret;
}

