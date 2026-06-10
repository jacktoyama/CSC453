CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -lm

OBJS    = libDisk.o tinyFS.o

all: tinyFSDemo diskTest

tinyFSDemo: $(OBJS) tinyFSDemo.o
	$(CC) $(CFLAGS) -o tinyFSDemo $(OBJS) tinyFSDemo.o $(LDFLAGS)

diskTest: $(OBJS) diskTest.o
	$(CC) $(CFLAGS) -o diskTest $(OBJS) diskTest.o $(LDFLAGS)

libDisk.o: libDisk.c libDisk.h
	$(CC) $(CFLAGS) -c libDisk.c

tinyFS.o: tinyFS.c tinyFS.h tinyFS_errno.h libDisk.h
	$(CC) $(CFLAGS) -c tinyFS.c

tinyFSDemo.o: tinyFSDemo.c tinyFS.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c tinyFSDemo.c

diskTest.o: diskTest.c libDisk.h
	$(CC) $(CFLAGS) -c diskTest.c

clean:
	rm -f *.o tinyFSDemo diskTest tinyFSDisk *.dsk

submission: libDisk.c libDisk.h tinyFS.c tinyFS.h tinyFS_errno.h tinyFSDemo.c diskTest.c tfsTest.c Makefile README
	tar -czf project4_submission.tar.gz libDisk.c libDisk.h tinyFS.c tinyFS.h tinyFS_errno.h tinyFSDemo.c diskTest.c tfsTest.c Makefile README