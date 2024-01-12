#ifndef SHOTGUND_DECL_H
#define SHOTGUND_DECL_H

// thread mananger config
#define HIGHWATERMARK    16
#define QUEUE_SIZE 128

// max parallel lookups
#define MAX_PAR 100

// default wait time in ms
#define DEFAULT_WAIT 500

#include "shotgund_sys.h"
#include "rfc1035.h"

struct QueueObj {
    struct AsyncDNSMessage msg;
    int msgLen;
    u_int16_t id;
    char addr[RFC1035_INET_ADDRSTRLEN];
    sockaddr_in in;
    int fd;
};

#include "log.h"

class Log;

#include "thread_queue.h"
#include "thread_manager.h"

class ResolverThread;
class Shotgund;
class ShotgundResolver;

#include "resolver_thread.h"
#include "shotgund.h"
#include "shotgund_resolver.h"

extern const Shotgund * __server;

#endif

