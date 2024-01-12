#ifndef BARD_SYSINC_H
#define BARD_SYSINC_H

#include <unistd.h>

#ifndef _POSIX_SYNCHRONIZED_IO
#define _POSIX_SYNCHRONIZED_IO
#endif

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
#include <fnmatch.h>
#include <utime.h>

#include <linux/types.h>
#include <linux/input.h>

#ifdef __cplusplus
extern "C" {
#endif

//char * chop_string(const char * str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
