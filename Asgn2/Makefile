CC 	= gcc

CFLAGS  = -Wall -g -I.c -Iinclude -Isrc

LD 	= gcc

LDFLAGS  = -Wall -g 

PROGS	= snakes nums hungry

SNAKEOBJS  = randomsnakes.o 

HUNGRYOBJS = hungrysnakes.o 

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) 

SRCS	= randomsnakes.c numbersmain.c hungrysnakes.c

HDRS	= 

EXTRACLEAN = core $(PROGS)

all: 	$(PROGS)

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(OBJS) *~ TAGS

snakes: demos/randomsnakes.o libLWP.a
	$(LD) $(LDFLAGS) -o snakes demos/randomsnakes.o -Llib64 -L. -lncurses -lsnakes -lLWP

hungry: demos/hungrysnakes.o libLWP.a libLWP.a
	$(LD) $(LDFLAGS) -o hungry demos/hungrysnakes.o -Llib64 -L. -lncurses -lsnakes -lLWP

nums: demos/numbersmain.o libLWP.a 
	$(LD) $(LDFLAGS) -o nums demos/numbersmain.o -Llib64 -L. -lLWP

hungrysnakes.o: include/lwp.h include/snakes.h

randomsnakes.o: include/lwp.h include/snakes.h

numbersmain.o: include/lwp.h

libLWP.a: src/lwp.c src/RoundRobin.c src/DoubleLinkedList.c demos/util.c src/magic64.S
	$(CC) $(CFLAGS) -Iinclude -Isrc -c src/RoundRobin.c src/DoubleLinkedList.c demos/util.c src/lwp.c src/magic64.S 
	ar r libLWP.a util.o lwp.o RoundRobin.o DoubleLinkedList.o magic64.o
	rm -rf lwp.o RoundRobin.o DoubleLinkedList.o util.o magic64.o

submission: src/lwp.c src/RoundRobin.c demos/util.c Makefile README
	tar -cf project2_submission.tar lwp.c RoundRobin.c Makefile README
	gzip project2_submission.tar
