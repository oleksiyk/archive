#include "bard_signal.h"

int bard_signal(int signum, void (*handler)(int))
{
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGSEGV);
    action.sa_handler = handler;
    action.sa_flags = SA_RESTART;

    return sigaction(signum,&action,NULL);
}

int bard_siginfo_signal(int signum, void (*handler)(int, siginfo_t *, void *))
{
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGSEGV);

    action.sa_sigaction = handler;
    action.sa_flags = SA_RESTART | SA_SIGINFO;

    return sigaction(signum,&action,NULL);
}

int bard_block_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return sigprocmask(SIG_BLOCK, &sa_mask, NULL);
}

int bard_unblock_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return sigprocmask(SIG_UNBLOCK, &sa_mask, NULL);
}

int bard_pthread_block_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return pthread_sigmask(SIG_BLOCK, &sa_mask, NULL);
}

int bard_pthread_unblock_signal(int signum)
{
    sigset_t sa_mask;

    sigemptyset(&sa_mask);
    sigaddset(&sa_mask, signum);

    return pthread_sigmask(SIG_UNBLOCK, &sa_mask, NULL);
}

int bard_sleep(long sec, long usec)
{
    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    return select(0,NULL,NULL,NULL,&tv);
}

int bard_detach(const char * cdir)
{
   pid_t pid = 0;

   bard_signal(SIGTTOU, SIG_IGN);
   bard_signal(SIGTTIN, SIG_IGN);
   bard_signal(SIGTSTP, SIG_IGN);

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

