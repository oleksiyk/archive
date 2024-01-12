#include "log.h"

const char * Log::typeNames[] = {"critical", "error", "warning", "notice", "info"};

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
        error(0, "Log: fopen('%s') failed: %s", fileName, strerror(errno));
        return -1;
    }

    if(fp != stderr){
        (void) fclose(fp);
    }

    fp = newfp;

    setlinebuf(fp);

    return 0;
}

int Log::vlog(Log::Type type, const char * msg, va_list ap)const
{
    time_t t = time(0);

    int len = 60+strlen(msg)+strlen(Log::typeNames[type]);
    char * str = (char*)malloc(len);

    int f = strftime(str, len, "[%a %e %b %Y %T]", localtime(&t));

    f+=snprintf(str+f, len-f, " [%s] %s\n", Log::typeNames[type], msg);
    str[f] = 0;

    vfprintf(fp, str, ap);
    
    free(str);

    return 0;
}

int Log::log(Log::Type type, u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = vlog(type, msg, args);

    va_end(args);
    
    return ret;
}

int Log::error(u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = vlog(Log::MESSAGE_ERROR, msg, args);

    va_end(args);
    
    return ret;
}

int Log::critical(u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = vlog(Log::MESSAGE_CRITICAL, msg, args);

    va_end(args);
    
    return ret;
}

int Log::warning(u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = vlog(Log::MESSAGE_WARNING, msg, args);

    va_end(args);
    
    return ret;
}

int Log::notice(u_int8_t level, const char * msg,  ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = vlog(Log::MESSAGE_NOTICE, msg, args);

    va_end(args);
    
    return ret;
}

int Log::info(u_int8_t level, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = vlog(Log::MESSAGE_INFO, msg, args);

    va_end(args);
    
    return ret;
}

