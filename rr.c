#include "lwp.h"
#include <stdlib.h>
#include <stdio.h>
#include "rr.h"

static thread qhead=NULL;
static int advance=FALSE;
#define tnext sched_one
#define tprev sched_two


static void rr_admit(thread new) {

  /* add to queue */
  if ( qhead ) {
    new->tnext = qhead;
    new->tprev = qhead->tprev;
    new->tprev->tnext = new;
    qhead->tprev = new;
  } else {
    advance = FALSE;
    qhead = new;
    qhead->tnext = new;
    qhead->tprev = new;
  }

}

static void rr_remove(thread victim) {
  /* cut out of queue */
  victim->tprev->tnext = victim->tnext;
  victim->tnext->tprev = victim->tprev;

  /* what if it were qhead? */
  if ( victim == qhead ) {
    if ( victim->tnext != victim)
      qhead = victim->tprev;  /* preserve who would've been next */
    else
      qhead = NULL;
  }
}

static thread rr_next() {

  if ( qhead ) {
    if ( advance  )
      qhead = qhead->tnext;
    else
      advance = TRUE;
  }

  return qhead;
}

struct scheduler rr_publish = {NULL, NULL, rr_admit,rr_remove,rr_next};
scheduler RoundRobin = &rr_publish;

/*********************************************************/
__attribute__ ((unused)) void
dpl() {
  thread l;
  if ( !qhead )
    fprintf(stderr,"qhead is NULL\n");
  else {
    fprintf(stderr,"queue:\n");
    l = qhead;
    do {
      fprintf(stderr,"  (tid=%lu tnext=%p tprev=%p)\n", l->tid,l->tnext,
              l->tprev);
      l=l->tnext;
    } while ( l != qhead ) ;
    fprintf(stderr,"\n");
  }
}

