/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#include "history.h"

ProbedHistory::ProbedHistory():
    fp(stderr)
{
};

ProbedHistory::~ProbedHistory()
{
    if(fp != stderr){
        (void) fclose(fp);
    }
}

int ProbedHistory::initialize(const char * fileName)
{
    FILE * newfp;

    newfp = fopen(fileName, "a+");

    if(newfp == NULL){
        log(0, "History: fopen('%s') failed: %s", fileName, strerror(errno));
        return -1;
    }

    if(fp != stderr){
        (void) fclose(fp);
    }

    fp = newfp;

    setlinebuf(fp);

    return 0;
}

int ProbedHistory::vlog(const char * msg, va_list ap)const
{
    time_t t = time(0);

    int len = 60+strlen(msg);
    char * str = (char*)malloc(len);

    int f = strftime(str, len, "%b %e %Y %H:%M:%S", localtime(&t));

    f+=snprintf(str+f, len-f, " %s\n", msg);
    str[f] = 0;

    vfprintf(fp, str, ap);
    /*
    if(_cmdForeground && fp != stderr){
        vfprintf(stderr, str, ap);
    }
    */

    free(str);

    return 0;
}

int ProbedHistory::log(const char * msg, ...)const
{
    va_list args;
    va_start(args, msg);

    int ret = vlog(msg, args);

    va_end(args);

    return ret;
}

