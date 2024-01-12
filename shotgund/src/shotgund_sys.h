#ifndef SHOTGUND_SYSINC_H
#define SHOTGUND_SYSINC_H

#include <unistd.h>

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#ifndef _POSIX_SYNCHRONIZED_IO
#define _POSIX_SYNCHRONIZED_IO
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <grp.h>
#include <pwd.h>
#include <syslog.h>
#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <fnmatch.h>
#include <utime.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/ppp_defs.h>
#include <net/if.h>
#include <net/if_ppp.h>

#include <regex.h>

extern int _cmdDebugLevel;
extern char * _cmdUser;
extern int _cmdForeground;
extern int _cmdPort;

#ifdef __cplusplus
extern "C" {
#endif

//char * chop_string(const char * str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
