#ifndef BARD_SIGNAL_H
#define BARD_SIGNAL_H

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
int bard_signal(int signum, void (*handler)(int));

/* install siginfo_t capable signal handler */
int bard_siginfo_signal(int signum, void (*handler)(int, siginfo_t *, void *));

/* portable implementation of sleep() */
int bard_sleep(long sec, long usec);

/* blocks signal */
int bard_block_signal(int signum);

/* unblocks signal */
int bard_unblock_signal(int signum);

/* blocks signal */
int bard_pthread_block_signal(int signum);

/* unblocks signal */
int bard_pthread_unblock_signal(int signum);

/* detach from the controlling terminal and run  in the background as system daemon */
int bard_detach(const char * cdir);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
