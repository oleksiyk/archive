/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#include "dac_signal.h"

int dac_signal(int signum, void (*handler)(int))
{
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGSEGV);
    action.sa_handler = handler;
    action.sa_flags = SA_RESTART;

    return sigaction(signum,&action,NULL);
}

int dac_siginfo_signal(int signum, void (*handler)(int, siginfo_t *, void *))
{
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGSEGV);

    action.sa_sigaction = handler;
    action.sa_flags = SA_RESTART | SA_SIGINFO;

    return sigaction(signum,&action,NULL);
}

int dac_block_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return sigprocmask(SIG_BLOCK, &sa_mask, NULL);
}

int dac_unblock_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return sigprocmask(SIG_UNBLOCK, &sa_mask, NULL);
}

int dac_pthread_block_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return pthread_sigmask(SIG_BLOCK, &sa_mask, NULL);
}

int dac_pthread_unblock_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return pthread_sigmask(SIG_UNBLOCK, &sa_mask, NULL);
}

int dac_sleep(long sec, long usec)
{
    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    return select(0,NULL,NULL,NULL,&tv);
}

int dac_detach(const char * cdir)
{
   pid_t pid = 0;

   dac_signal(SIGTTOU, SIG_IGN);
   dac_signal(SIGTTIN, SIG_IGN);
   dac_signal(SIGTSTP, SIG_IGN);

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

 /* return checksum in low-order 16 bits */
int in_cksum(register u_short * ptr, register int nbytes)
{
        register long           sum;            /* assumes long == 32 bits */
        u_short                 oddbyte;
        register u_short        answer;         /* assumes u_short == 16 bits */

        /*
         * Our algorithm is simple, using a 32-bit accumulator (sum),
         * we add sequential 16-bit words to it, and at the end, fold back
         * all the carry bits from the top 16 bits into the lower 16 bits.
         */

        sum = 0;
        while (nbytes > 1)  {
                sum += *ptr++;
                nbytes -= 2;
        }

                                /* mop up an odd byte, if necessary */
        if (nbytes == 1) {
                oddbyte = 0;            /* make sure top half is zero */
                *((u_char *) &oddbyte) = *(u_char *)ptr;   /* one byte only */
                sum += oddbyte;
        }

        /*
         * Add back carry outs from top 16 bits to low 16 bits.
         */

        sum  = (sum >> 16) + (sum & 0xffff);    /* add high-16 to low-16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;          /* ones-complement, then truncate to 16 bits */
        return(answer);
}
