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
        log(0, 0,0, "Log: fopen('%s') failed: %s", fileName, strerror(errno));
        return -1;
    }

    if(fp != stderr){
        (void) fclose(fp);
    }

    fp = newfp;

    setlinebuf(fp);

    return 0;
}

int Log::vlog(int query_id, int query_subid, const char * msg, va_list ap)const
{
    time_t t = time(0);
    struct timeval tv;

    int len = 60+strlen(msg);
    char * str = (char*)malloc(len);

    int f = strftime(str, len, "%Y%m%d %H:%M:%S", localtime(&t));

    gettimeofday(&tv, NULL);

    if(query_id != 0 && query_subid != 0){
        f+=snprintf(str+f, len-f, ".%.3ld [%.6d.%.3d] %s\n", tv.tv_usec/1000, query_id, query_subid, msg);
    } else if(query_subid == 0 && query_id != 0){
        f+=snprintf(str+f, len-f, ".%.3ld [%.6d] %s\n", tv.tv_usec/1000, query_id, msg);
    } else {
        f+=snprintf(str+f, len-f, ".%.3ld %s\n", tv.tv_usec/1000, msg);
    }
    str[f] = 0;

    vfprintf(fp, str, ap);
    if(_cmdForeground && fp != stderr){
        vfprintf(stderr, str, ap);
    }

    free(str);

    return 0;
}

int Log::log(u_int8_t level, int query_id, int query_subid, const char * msg, ...)const
{
    if(level > loggingLevel)
        return 0;

    va_list args;
    va_start(args, msg);

    int ret = vlog(query_id, query_subid, msg, args);

    va_end(args);

    return ret;
}

