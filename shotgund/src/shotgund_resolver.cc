#include "shotgund_resolver.h"

void ShotgundResolver::error(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    size_t len = strlen(fmt) + 50;

    char * buf = (char*)mempool->alloc(len);

    (void) snprintf(buf, len, "%s", fmt);

    buf[len-1] = 0;

    log->vlog(obj!=NULL?obj->id:0, subid, buf, args);

    va_end(args);
}

