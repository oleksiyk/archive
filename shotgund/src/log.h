#ifndef SHOTGUND_LOG_H
#define SHOTGUND_LOG_H

#include "shotgund_sys.h"

class Log
{
private:
    u_int8_t loggingLevel;
    FILE * fp;

public:
    Log(u_int8_t _loggingLevel);
    virtual ~Log();

    int vlog(int query_id, int query_subid, const char * msg, va_list ap)const;
    int log(u_int8_t level, int query_id, int query_subid, const char * msg, ...)const;

    void setLoggingLevel(int _loggingLevel){ loggingLevel = _loggingLevel; }

    int initialize(const char * fileName);
};

#endif
