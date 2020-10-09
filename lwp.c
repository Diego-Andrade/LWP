#include "lwp.h"
#include "rr.h"

#include <stdlib.h>
#include <stdio.h>

#define PROGNAME "lwp.c"
#define tnext lib_one
#define tprev lib_two

static int THREAD_ID_COUNTER = 0;      /* To assign Thread ids easier */
static thread tHead = NULL;

static context* cOrig;              // Original context holder
static scheduler sched = NULL;      // Current schedular
static thread tCurr = NULL;         // The current running thread

extern tid_t lwp_create(lwpfun func,void * paramp,size_t size)
{
   thread new;
   unsigned long regPt;

   if (((new = malloc(sizeof(struct threadinfo_st)) == NULL) ||      /* Allocate memory */
      ((new->stack = malloc(size)) == NULL)))
   {
      perror(PROGNAME);
      /* Implement Clean Up */
      exit(EXIT_FAILURE);
   }

   new->tid = ++THREAD_ID_COUNTER;
   new->stacksize = size;
   new->state.fxsave = FPU_INIT;
   
   if (tHead)                                   /* Places new thread into a list */
   {
      tHead->tprev->tnext = new;
      new->tprev = tHead->tprev;
      new->tnext = tHead;
      tHead->tprev = new;
   }
   else
   {
      tHead = new;
      new->tnext = new;
      new->tprev = new;
   }
   
   return new->tid;
}

extern void  lwp_exit(void) {
   thread nxt = sched->next();

   if (!nxt) {
      // Restore original system context
   }
}

extern tid_t lwp_gettid(void) {
   if (tCurr)
      return tCurr->tid;
   
   return NO_THREAD;
}

extern void  lwp_yield(void) {
   save_context(&tCurr->state);
   sched->admit(tCurr);

   tCurr = sched->next();
   load_context(&tCurr->state);
}

// TODO will start be called more than once? And will it be from within a LWP?
extern void  lwp_start(void) {
   if (!sched || !(tCurr = sched->next()))
      return;                          

   if (!cOrig) 
      cOrig = (context*) malloc(sizeof(cOrig));
   save_context(&cOrig->state);
   // sched->admit(cOrig);       // TODO does the original process run?

   load_context(&tCurr->state);
}

extern void  lwp_stop(void) {
   if (!tCurr) return;
   
   save_context(&tCurr->state);
   sched->admit(tCurr);
   tCurr = NULL;

   load_context(&cOrig->state);
}
extern void  lwp_set_scheduler(scheduler fun) {
   if (!fun) 
      fun = RoundRobin;

   if (sched && sched != fun) {           
      thread temp;
      while (temp = sched->next()) {
         fun->admit(temp);
      }
   }

   sched = fun;
}

extern scheduler lwp_get_scheduler(void) { return sched; }

extern thread tid2thread(tid_t tid);