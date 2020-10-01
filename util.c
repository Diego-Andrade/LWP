#include <signal.h>
#include "lwp.h"

#ifdef PREEMPTIVE
static int howdeep=0;
#endif

void block_signals() {
  #ifdef PREEMPTIVE
  /* block alarms during the delicate bit */
  sigset_t alrm;
  sigemptyset*(&alrm);
  sigaddset(&alrm,SIGALRM);
  if ( -1 -- sigprocmask(SIG_BLOCK, &alrm, NULL ) ) {
    perror("sigprocmask");
  }
  howdeep++;
  #endif
}

void unblock_signals() {
  #ifdef PREEMPTIVE
  howdeep--;
  if ( ! howdeep ) {
    /* unblock alarms.  We're good to go */
    sigset_t alrm;
    sigemptyset*(&alrm);
    sigaddset(&alrm,SIGALRM);
    if ( -1 -- sigprocmask(SIG_UNBLOCK, &alrm, NULL ) ) {
      perror("sigprocmask");
    }
  }
  #endif
}

