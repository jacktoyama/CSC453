#ifndef LIB_DISK_H
#define LIB_DISK_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#define BLOCKSIZE 256

// Disk Emulator Functions 
int openDisk(char *filename, int nBytes);
int closeDisk(int disk);
int readBlock(int disk, int bNum, void *block);
int writeBlock(int disk, int bNum, void *block);


#endif