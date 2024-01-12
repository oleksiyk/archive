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

#include "op_signal.h"

int outpost_signal(int signum, void (*handler)(int))
{
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGSEGV);
    action.sa_handler = handler;
    action.sa_flags = SA_RESTART;

    return sigaction(signum,&action,NULL);
}

int outpost_siginfo_signal(int signum, void (*handler)(int, siginfo_t *, void *))
{
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGSEGV);

    action.sa_sigaction = handler;
    action.sa_flags = SA_RESTART | SA_SIGINFO;

    return sigaction(signum,&action,NULL);
}

int outpost_block_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return sigprocmask(SIG_BLOCK, &sa_mask, NULL);
}

int outpost_unblock_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return sigprocmask(SIG_UNBLOCK, &sa_mask, NULL);
}

int outpost_pthread_block_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return pthread_sigmask(SIG_BLOCK, &sa_mask, NULL);
}

int outpost_pthread_unblock_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return pthread_sigmask(SIG_UNBLOCK, &sa_mask, NULL);
}

int outpost_sleep(long sec, long usec)
{
    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    return select(0,NULL,NULL,NULL,&tv);
}

int outpost_detach(const char * cdir)
{
   pid_t pid = 0;

   outpost_signal(SIGTTOU, SIG_IGN);
   outpost_signal(SIGTTIN, SIG_IGN);
   outpost_signal(SIGTSTP, SIG_IGN);

   if(cdir != NULL){
      if(chdir(cdir) == -1){
         fprintf(stderr, "chdir('%s') failed: %s", cdir, strerror(errno));
         return -1;
      }
   } /* else {
      if(chdir("/") == -1){
         fprintf(stderr, "chdir('/') failed: %s", strerror(errno));
         return -1;
      }
   } */

   if (getppid() != 1)
      pid = fork();

   if (pid > 0) {
      exit(0);
   } else if (pid == -1) {
      fprintf(stderr, "fork() failed: %s\n", strerror(errno));
      return -1;
   }

   if (setsid() == -1) {
      fprintf(stderr, "setsid() failed: %s\n", strerror(errno));
      return -1;
   }

   if (freopen("/dev/null", "r", stdin) == NULL){
      fprintf(stderr, "unable to replace stdin with /dev/null: %s\n", strerror(errno));
      return -1;
   }

   if (freopen("/dev/null", "w", stdout) == NULL){
      fprintf(stderr, "unable to replace stdout with /dev/null: %s\n", strerror(errno));
      return -1;
   }

   /*
   * NOTE:! stderr is replaced here with /dev/null, so any output to it will not work by now
   */
   if (freopen("/dev/null", "w", stderr) == NULL){
      fprintf(stderr, "unable to replace stdout with /dev/null: %s\n", strerror(errno));
      return -1;
   }

   return 0;
}

