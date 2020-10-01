CC 	= gcc

CFLAGS  = -Wall -g -I .

LD 	= gcc

LDFLAGS  = -Wall -g 

PROGS	= snakes nums hungry

SNAKEOBJS  = snakemain.o 

HUNGRYOBJS = hungrymain.o 

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) 

SRCS	= snakemain.c numbersmain.c hungrymain.c

HDRS	= 

EXTRACLEAN = core $(PROGS)

all: 	$(PROGS)

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(OBJS) *~ TAGS

snakes: snakemain.o libLWP.a libsnakes.a
	$(LD) $(LDFLAGS) -o snakes snakemain.o -L. -lncurses -lsnakes -lLWP

hungry: hungrymain.o libLWP.a libsnakes.a
	$(LD) $(LDFLAGS) -o hungry hungrymain.o -L. -lncurses -lsnakes -lLWP

nums: numbersmain.o libLWP.a 
	$(LD) $(LDFLAGS) -o nums numbersmain.o -L. -lLWP

hungrymain.o: lwp.h snakes.h

snakemain.o: lwp.h snakes.h

numbermain.o: lwp.h

libLWP.a: lwp.c rr.c util.c 
	gcc -c rr.c util.c lwp.c magic64.S
	ar r libLWP.a util.o lwp.o rr.o magic64.o
	rm lwp.o

submission: lwp.c Makefile README
	tar -cf project2_submission.tar lwp.c Makefile README
	gzip project2_submission.tar
