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

#ifndef OUTPOST_SIGNAL_H
#define OUTPOST_SIGNAL_H

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
int outpost_signal(int signum, void (*handler)(int));

/* install siginfo_t capable signal handler */
int outpost_siginfo_signal(int signum, void (*handler)(int, siginfo_t *, void *));

/* portable implementation of sleep() */
int outpost_sleep(long sec, long usec);

/* blocks signal */
int outpost_block_signal(int signum);

/* unblocks signal */
int outpost_unblock_signal(int signum);

/* blocks signal */
int outpost_pthread_block_signal(int signum);

/* unblocks signal */
int outpost_pthread_unblock_signal(int signum);

/* detach from the controlling terminal and run  in the background as system daemon */
int outpost_detach(const char * cdir);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
