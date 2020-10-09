#include "lwp.h"
#include <stdlib.h>
#include <stdio.h>

#define PROGNAME "lwp.c"
#define tnext lib_one
#define tprev lib_two

static int THREAD_ID_COUNTER = 0;      /* To assign Thread ids easier */
static thread tHead = NULL;

extern tid_t lwp_create(lwpfun func,void *paramp,size_t size)
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

extern void  lwp_exit(void);
extern tid_t lwp_gettid(void);
extern void  lwp_yield(void);
extern void  lwp_start(void);
extern void  lwp_stop(void);
extern void  lwp_set_scheduler(scheduler fun);
extern scheduler lwp_get_scheduler(void);
extern thread tid2thread(tid_t tid);