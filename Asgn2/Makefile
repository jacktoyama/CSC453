CC = gcc
CFLAGS = -Wall -g -I. -Iinclude -Isrc -Idemos
LD = gcc
LDFLAGS = -Wall -g -Llib64 -L. -Wl,-rpath,lib64

PROGS = snakes hungry nums

all: $(PROGS)

snakes: randomsnakes.o libLWP.a
	$(LD) $(LDFLAGS) -o $@ $< -lsnakes -lncurses -lLWP

hungry: hungrysnakes.o libLWP.a
	$(LD) $(LDFLAGS) -o $@ $< -lsnakes -lncurses -lLWP

nums: numbersmain.o libLWP.a
	$(LD) $(LDFLAGS) -o $@ $< -lLWP

randomsnakes.o: demos/randomsnakes.c include/lwp.h include/snakes.h
	$(CC) $(CFLAGS) -c $< -o $@

hungrysnakes.o: demos/hungrysnakes.c include/lwp.h include/snakes.h
	$(CC) $(CFLAGS) -c $< -o $@

numbersmain.o: demos/numbersmain.c include/lwp.h
	$(CC) $(CFLAGS) -c $< -o $@

libLWP.a: src/RoundRobin.c src/DoubleLinkedList.c src/util.c src/lwp.c src/magic64.S
	$(CC) $(CFLAGS) -c src/RoundRobin.c src/DoubleLinkedList.c src/util.c src/lwp.c src/magic64.S
	ar rcs $@ RoundRobin.o DoubleLinkedList.o util.o lwp.o magic64.o
	rm -f RoundRobin.o DoubleLinkedList.o util.o lwp.o magic64.o

clean:
	rm -f $(PROGS) randomsnakes.o hungrysnakes.o numbersmain.o libLWP.a RoundRobin.o DoubleLinkedList.o util.o lwp.o magic64.o

.PHONY: all clean
