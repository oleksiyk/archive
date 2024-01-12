#ifndef SHOTGUND_RESOLVER_H
#define SHOTGUND_RESOLVER_H

#include "log.h"
#include "dotconfpp.h"
#include "rfc1035.h"
#include "decl.h"

class ShotgundResolver : public AsyncDNSResolver
{
private:
    virtual void error(const char * fmt, ...);
    const Log * log;
    const QueueObj * obj;
    int subid;

public:
    bool skip;
    time_t waitms;
    int weight;
    bool norewrite;
    ShotgundResolver(const Log * _log):log(_log),subid(0),skip(false){};
    void setQueueObj(const QueueObj * _obj, int _subid){ obj = _obj; subid = _subid; skip = false; waitms = DEFAULT_WAIT; weight=1; norewrite = false; }
    int getSubid()const{ return subid; }
};


#endif
