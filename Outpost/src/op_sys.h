/*  Copyright (C) 2003 FOSS-On-Line <http://www.foss.kharkov.ua>,
*   Aleksey Krivoshey <krivoshey@users.sourceforge.net>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef OUTPOST_SYSINC_H
#define OUTPOST_SYSINC_H

#include <unistd.h>

#ifndef _POSIX_SYNCHRONIZED_IO
#define _POSIX_SYNCHRONIZED_IO
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/un.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <sys/sysctl.h>
#include <grp.h>
#include <pwd.h>
#include <syslog.h>
#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <dlfcn.h>
#include <fnmatch.h>
#include <utime.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_ppp.h>

#include <regex.h>

/* for systems which do not have fdatasync (FreeBSD) */
#ifndef fdatasync
#define fdatasync fsync
#endif

#ifdef OUTPOST_IPV6
    #define OUTPOST_sockaddr_in sockaddr_in6
    #define OUTPOST_AF_INET AF_INET6
    #define OUTPOST_PF_INET PF_INET6
    #define OUTPOST_sin_family sin6_family
    #define OUTPOST_sin_port sin6_port
    #define OUTPOST_sin_addr sin6_addr 
    #define OUTPOST_in_addr in6_addr
    #define OUTPOST_INET_ADDRSTRLEN INET6_ADDRSTRLEN
#else
    #define OUTPOST_sockaddr_in sockaddr_in
    #define OUTPOST_AF_INET AF_INET
    #define OUTPOST_PF_INET PF_INET
    #define OUTPOST_sin_family sin_family
    #define OUTPOST_sin_port sin_port
    #define OUTPOST_sin_addr sin_addr
    #define OUTPOST_in_addr in_addr
    #define OUTPOST_INET_ADDRSTRLEN INET_ADDRSTRLEN
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef OUTPOST_IPV6
void outpost_ipv6to4(struct in_addr *ip4, const struct in6_addr *ip6);
void outpost_ipv4to6(struct in6_addr *ip6, const struct in_addr *ip4);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
