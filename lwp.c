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

void pushToStack(unsigned long item, unsigned long **sp)
{
   **sp = item;
   (*sp)++;
}

void printStack(unsigned long* stack, int start, int end) {
   stack += end;
   for (int i = end; i > start; --i, --stack) {
      printf("%ld : %ld\n", (unsigned long)stack,*stack);
   }
}

void printRFile(rfile* rf) {
   unsigned long* r = (unsigned long*) r;
   for (int i = 0 ; r <= &rf->r15; i++, r++) {
      printf("%ld r%d: %ld\n", (unsigned long)r, i, *r);
   }
   printf("\n");
}

extern tid_t lwp_create(lwpfun func,void * paramp,size_t size)
{
   thread new;
   unsigned long *regPt;
   unsigned long *parameters = (unsigned long*) paramp;
   // printf("thread init\n");
   if ( (new = (thread) malloc(sizeof(context))) == NULL ||      /* Allocate memory */
      (new->stack = (unsigned long*) malloc(size)) == NULL)
   {
      perror(PROGNAME);
      /* Implement Clean Up */
      return -1;
   }
   // printf("thread malloced stack\n");

   /* Initialization */
   new->tid = ++THREAD_ID_COUNTER;
   new->stacksize = size;
   new->state.fxsave = FPU_INIT;    
   new->state.rsp = ((unsigned long)new->stack) + size;

   new->state.rsp = (unsigned long) (((unsigned long*)new->state.rsp) - 1);      // move rsp to end of stack
   *((unsigned long*)new->state.rsp) = (unsigned long) lwp_exit;    // return address for exiting thread
   new->state.rsp = (unsigned long) (((unsigned long*)new->state.rsp) - 1);      //push function address
   *((unsigned long*)new->state.rsp) = (unsigned long) func;
   new->state.rsp = (unsigned long) (((unsigned long*)new->state.rsp) - 1);   // push stack location for rbp
   *((unsigned long*)new->state.rsp) = ((unsigned long)new->stack) + size;
   new->state.rbp = new->state.rsp;    

   // unsigned long a = 2;          
   // unsigned long* rsp = a;       // This should not be allowed....
   // pushToStack(func, &new->state.rsp);    // Incorrect because of above

   // printf("thread param init\n");
   
   new->state.rdi = (unsigned long) ((unsigned long*) paramp);
   new->state.rsi = (unsigned long) ((unsigned long*) paramp + 1);
   new->state.rdx = (unsigned long) ((unsigned long*) paramp + 2);
   new->state.rcx = (unsigned long) ((unsigned long*) paramp + 3);
   new->state.r8  = (unsigned long) ((unsigned long*) paramp + 4);
   new->state.r9  = (unsigned long) ((unsigned long*) paramp + 5);
   
   // while (parameters != NULL)
   //    pushToStack(parameters++, &new->state.rsp);

   // printf("thread link list\n");

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

   // printStack(new->stack, size - 5, size-1);
   // printf("\nrbp: %ld\n", new->state.rbp);
   // printf("rsp: %ld\n", new->state.rsp);
   // printf("stack end: %ld\n", (unsigned long)(new->stack+size-1));

   // printRFile(&new->state);

   // printf("\ndone with thread %ld\n\n", new->tid);

   return new->tid;
}

extern void  lwp_exit(void) {
   sched->remove(tCurr);

   free(tCurr->stack);
   free(tCurr);
   thread tCurr = sched->next();
   if (!tCurr) {
      // Restore original system context
      load_context(&cOrig->state);
      free(cOrig);
      cOrig = NULL;
      return;
   }
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

// TODO will start be called more than once? And will it be from within a LWP?
extern void  lwp_start(void) {
   // printf("!!starting lwp!!\n");
   if (!sched || !(tCurr = sched->next()))
      return;                          

   if (!cOrig)
      cOrig = (context*) malloc(sizeof(cOrig));
   save_context(&cOrig->state);
   // sched->admit(cOrig);       // TODO does the original process run?
   // SetSP(tCurr->state.rsp);
   // printf("Loading %ld into context\n", tCurr->tid);
   load_context(&tCurr->state);
   

   // printStack(tCurr->stack, tCurr->stacksize - 5, tCurr->stacksize);
   // printf("\nrbp: %ld\n", tCurr->state.rbp);
   // printf("rsp: %ld\n", tCurr->state.rsp);
   // printf("stack end: %ld\n", (unsigned long)(tCurr->stack + tCurr->stacksize-1));

   // printf("Starting %ld\n", tCurr->tid);
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