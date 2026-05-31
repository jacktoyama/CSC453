#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#include "libDisk.h"

int openDisk(char *filename, int nBytes) {

	int diskSize = 0;

    FILE *disk = fopen(filename, "r");
	if (disk == NULL) {
		fprintf(stderr, "ERROR: fopen().\n");
		exit(1);
	}

	if (nBytes > BLOCKSIZE) {
		return EXIT_FAILURE;
	}

    if (nBytes % BLOCKSIZE != 0) {
        diskSize = nBytes - (nBytes % BLOCKSIZE);
    }

	
}