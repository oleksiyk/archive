/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#ifndef DAC_SIGNAL_H
#define DAC_SIGNAL_H

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
int dac_signal(int signum, void (*handler)(int));

/* install siginfo_t capable signal handler */
int dac_siginfo_signal(int signum, void (*handler)(int, siginfo_t *, void *));

/* portable implementation of sleep() */
int dac_sleep(long sec, long usec);

/* blocks signal */
int dac_block_signal(int signum);

/* unblocks signal */
int dac_unblock_signal(int signum);

/* blocks signal */
int dac_pthread_block_signal(int signum);

/* unblocks signal */
int dac_pthread_unblock_signal(int signum);

/* detach from the controlling terminal and run  in the background as system daemon */
int dac_detach(const char * cdir);

/* calculates icmp checksum */
int in_cksum(register u_short * ptr, register int nbytes);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
