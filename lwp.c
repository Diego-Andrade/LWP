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

void printStack(thread t) {
   unsigned long* stack = (unsigned long*)(((unsigned long)t->stack) + t->stacksize);
   for ( ; stack != t->stack - 1;  --stack) {
      printf("%p : %p\n", stack, (void*)*stack);
   }
}

void push(unsigned long *rsp, unsigned long val) {
   *rsp = (unsigned long) (((unsigned long*)*rsp) - 1);
   *(unsigned long*)*rsp = val;
}

extern tid_t lwp_create(lwpfun func,void * paramp,size_t size) 
{
   context *new;

   // printf("thread init\n");
   if ( (new = (context*) malloc(sizeof(context))) == NULL ||      /* Allocate memory */
      (new->stack = (unsigned long*) malloc(size * sizeof(unsigned long))) == NULL)
   {
      perror(PROGNAME);
      return -1;
   }

   /* Initialization */
   new->tid = ++THREAD_ID_COUNTER;
   new->stacksize = size;
   new->state.fxsave = FPU_INIT;    
   new->state.rsp = (unsigned long)(new->stack + size - 1);     
   new->state.rbp = new->state.rsp;                                           
                                                         // ---------------    
   push(&new->state.rsp, (unsigned long) lwp_exit);      //    lwp_exit
   push(&new->state.rsp, new->state.rbp);                //    old_rbp
   new->state.rbp = new->state.rsp;
   push(&new->state.rsp, (unsigned long) func);          //    func
   push(&new->state.rsp, new->state.rbp);                //    old_rbp
   new->state.rbp = new->state.rsp;   
   
   new->state.rdi = (unsigned long) ((unsigned long*) paramp);
   new->state.rsi = (unsigned long) ((unsigned long*) paramp + 1);
   new->state.rdx = (unsigned long) ((unsigned long*) paramp + 2);
   new->state.rcx = (unsigned long) ((unsigned long*) paramp + 3);
   new->state.r8  = (unsigned long) ((unsigned long*) paramp + 4);
   new->state.r9  = (unsigned long) ((unsigned long*) paramp + 5);
   
   // while (parameters != NULL)
   //    pushToStack(parameters++, &new->state.rsp);

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
   
   if (!sched) sched = RoundRobin;
   sched->admit(new);

   return new->tid;
}

extern void  lwp_exit(void) {
   sched->remove(tCurr);

   free(tCurr->stack);
   free(tCurr);
   thread tCurr = sched->next();
   if (!tCurr)
      load_context(&cOrig->state);     // Restore original system context
   load_context(&tCurr->state);
}

extern tid_t lwp_gettid(void) {
   if (tCurr)
      return tCurr->tid;
   
   return NO_THREAD;
}

extern void lwp_yield(void) {
   save_context(&tCurr->state);
   if (tCurr = sched->next())
      load_context(&tCurr->state);
}

extern void  lwp_start(void) {
   if (!sched || !(tCurr = sched->next()))
      return;                          

   if (!cOrig)
      cOrig = (context*) malloc(sizeof(context));
   save_context(&cOrig->state);
   load_context(&tCurr->state);
   free(cOrig);
   cOrig = NULL;
}

extern void  lwp_stop(void) {
   save_context(&tCurr->state);
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

extern thread tid2thread(tid_t tid)
{
   thread curr = tHead->tnext;

   while (curr != tHead && curr->tid != tid);

   if (curr->tid == tid)
      return curr;
   else
      return NULL;
}