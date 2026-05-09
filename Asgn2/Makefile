CC 	= gcc

CFLAGS  = -Wall -g -I. -Iinclude -Isrc -Idemos -Ilib64

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

#clean:	
#	rm -f $(OBJS) *~ TAGS
clean:
	rm -f $(OBJS) *~ TAGS liblwp.so libLWP.a snakes nums hungry *.o

snakes: randomsnakes.o libLWP.a 
	$(LD) $(LDFLAGS) -o snakes randomsnakes.o -L. -lncurses -lsnakes -lLWP

hungry: hungrysnakes.o libLWP.a 
	$(LD) $(LDFLAGS) -o hungry hungrysnakes.o -L. -lncurses -lsnakes -lLWP

nums: numbersmain.o libLWP.a 
	$(LD) $(LDFLAGS) -o nums numbersmain.o -L. -lLWP

hungrysnakes.o: include/lwp.h include/snakes.h

randomsnakes.o: include/lwp.h include/snakes.h

numbersmain.o: include/lwp.h

libLWP.a: src/lwp.c src/RoundRobin.c src/DoubleLinkedList.c src/util.c src/magic64.S
	$(CC) $(CFLAGS) -c src/RoundRobin.c src/DoubleLinkedList.c src/util.c src/lwp.c src/magic64.S
	ar rcs libLWP.a RoundRobin.o DoubleLinkedList.o util.o lwp.o magic64.o
	rm -f RoundRobin.o DoubleLinkedList.o util.o lwp.o magic64.o

liblwp.so: src/lwp.c src/RoundRobin.c src/DoubleLinkedList.c src/util.c src/magic64.S
	$(CC) -fPIC $(CFLAGS) -c src/RoundRobin.c src/DoubleLinkedList.c src/util.c src/lwp.c src/magic64.S
	$(CC) -shared -o liblwp.so util.o lwp.o RoundRobin.o DoubleLinkedList.o magic64.o
	rm -f util.o lwp.o RoundRobin.o DoubleLinkedList.o magic64.o

submission: lwp.c rr.c util.c Makefile README
	tar -cf project2_submission.tar lwp.c rr.c Makefile README
	gzip project2_submission.tar
