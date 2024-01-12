/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#ifndef PROBED_SIGNAL_H
#define PROBED_SIGNAL_H

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* install signal handler in BSD style */
int probed_signal(int signum, void (*handler)(int));

/* install siginfo_t capable signal handler */
int probed_siginfo_signal(int signum, void (*handler)(int, siginfo_t *, void *));

/* portable implementation of sleep() */
int probed_sleep(long sec, long usec);

/* blocks signal */
int probed_block_signal(int signum);

/* unblocks signal */
int probed_unblock_signal(int signum);

/* blocks signal */
int probed_pthread_block_signal(int signum);

/* unblocks signal */
int probed_pthread_unblock_signal(int signum);

/* detach from the controlling terminal and run  in the background as system daemon */
int probed_detach(const char * cdir);

/* calculates icmp checksum */
int in_cksum(register u_short * ptr, register int nbytes);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
