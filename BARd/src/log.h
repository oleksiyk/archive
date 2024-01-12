#ifndef BARD_LOG_H
#define BARD_LOG_H

#include "bard_sys.h"

class Log
{
private:
    u_int8_t loggingLevel;
    FILE * fp;
    
    enum Type {
        MESSAGE_CRITICAL = 0,
        MESSAGE_ERROR = 1,
        MESSAGE_WARNING = 2,
        MESSAGE_NOTICE = 3,
        MESSAGE_INFO = 4,
    };
    static const char * typeNames[];
    
    int vlog(Log::Type type, const char * msg, va_list ap)const;

public:
    Log(u_int8_t _loggingLevel);
    virtual ~Log();

    int log(Log::Type type, u_int8_t level, const char * msg, ...)const;
    int error(u_int8_t level, const char * msg, ...)const;
    int critical(u_int8_t level, const char * msg, ...)const;
    int warning(u_int8_t level, const char * msg, ...)const;
    int notice(u_int8_t level, const char * msg,  ...)const;
    int info(u_int8_t level, const char * msg, ...)const;

    void setLoggingLevel(int _loggingLevel){ loggingLevel = _loggingLevel; }
    
    int initialize(const char * fileName);
};

#endif
