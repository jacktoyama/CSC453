#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#include "libDisk.h"

int openDisk(char *filename, int nBytes) {
	int diskSize = 0;
    int disk;

	// Content is not overwritten
	if (nBytes == 0) {
		// Open an existing disk
		disk = open(filename, O_RDWR); // O_RDWR for writeDisk later
		if (disk < 0) {
			fprintf(stderr, "ERROR: open().\n");
			return -1;
		}
		return disk;
	}

	// If nBytes less than BLOCKSIZE, return failure
	if (nBytes < BLOCKSIZE) {
		return -1;
	}
	
	// nBytes is not a multiple of BLOCKSIZE check
	diskSize = nBytes - (nBytes % BLOCKSIZE); // Closest multiple of BLOCKSIZE that is lower than nByte
	
	disk = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666); // O_TRUNC erases data in a file
	if (disk < 0) {
		fprintf(stderr, "ERROR: open().\n");
		return -1;	
	}
	if (ftruncate(disk, diskSize) < 0) { // Truncate the disk to diskSize, which is 256 aligned
		fprintf(stderr, "ERROR: ftruncate().\n");
		close(disk);
		return -1;
	}
	return disk; // file descriptor 

}

// Close an existing disk
int closeDisk(int disk) {
	if (close(disk) != 0) {
		return -1;
	}
	return 0;
}

// Read a block of bytes into a local buffer
int readBlock(int disk, int bNum, void *block) {
	// Read this block of code
	if (lseek(disk, bNum * BLOCKSIZE, SEEK_SET) < 0) {
		fprintf(stderr, "ERROR: lseek().\n");
		return -1;
	}

	// Read into local buffer 
	ssize_t bytesRead = read(disk, block, BLOCKSIZE);
	// if (bytesRead > 0) {
	// 	block[bytesRead] = '\0'; // null-terminate
	// } else {
	// 	fprintf(stderr, "ERROR: read().\n");
	// 	return -1;
	// }

	if (bytesRead < 0) {
		fprintf(stderr, "ERROR: read().\n");
		return -1;
	}
	return 0;
}

// Write block
int writeBlock(int disk, int bNum, void *block) {
	// Go to this block of data
	if (lseek(disk, bNum * BLOCKSIZE, SEEK_SET) < 0) {
		fprintf(stderr, "ERROR: lseek().\n");
		return -1;
	}

	// Write to this part of the disk
	ssize_t bytesWrite = write(disk, block, BLOCKSIZE);
	if (bytesWrite == -1) {
		fprintf(stderr, "ERROR: write().\n");
		return -1;
	}

	return 0;
}