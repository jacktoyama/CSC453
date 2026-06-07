#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <sys/errno.h>

#include "tinyFS.h"
#include "libDisk.h"
#include "tinyFS_errno.h"

// Global variables
static int mountedDisk = TFS_NOT_MOUNTED;
static int numOfBlocks = 0;

// Open Table Entry Struct
typedef struct {
    int inUse;
    int inodeBlock; // inode block this file uses
    int filePointer; //current read/write pos inside the file
} OpenTableEntry;

// Array of files of type OpenTableEntry
static OpenTableEntry openFileTable[MAX_OPEN_FILE];

// Make a blank TinyFileSystem of size nBytes specified by 'filename'
int tfs_mkfs(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE) {
        return TFS_ERROR; 
    }
    int usableSize = nBytes - (nBytes % BLOCKSIZE);
    int numBlocks = usableSize / BLOCKSIZE; // 10240 / 256 = blocks
    numOfBlocks = numBlocks;

    // Oppen disk to mount to build an empty filesystem
    int fd = openDisk(filename, nBytes);

    // Build superblock
    unsigned char buildBlock[BLOCKSIZE]; // a block is 256 bytes 
    memset(buildBlock, 0, BLOCKSIZE); // Set every byte to 0 
    buildBlock[0] = 1;      // Superblock
    buildBlock[1] = 0x44;   // Magic number
    buildBlock[2] = 1;      // first free block
    buildBlock[3] = 0;      // empty

    writeBlock(fd, 0, buildBlock); // Write the superblock to file at block #0
    
    // Write the rest of the blocks
    for (int i = 0; i < numBlocks; i++) {
        memset(buildBlock, 0, BLOCKSIZE); // Initialize everything to 0
        buildBlock[0] = 4; // Free block since we're initializing
        buildBlock[1] = 0x44;
        if (i == numBlocks - 1) {
            buildBlock[2] = 0; // reaches the end of block, linked to superblock (not a free block)
        } else {
            buildBlock[2] = i+1; // Next free block
        }
        buildBlock[3] = 0;
        writeBlock(fd, i, buildBlock);
    }

    closeDisk(fd);
    return TFS_SUCCESS;
}

// Mount
int tfs_mount(char *diskname) {
    unsigned char verifyBlock[BLOCKSIZE];

    // Open file
    int disk = openDisk(diskname, 0); // Diks already opened 
    readBlock(disk, 0, verifyBlock);  // Read block 0 into buffer

    //  Verify it's type 1 and has 0x44
    if ((verifyBlock[0] != 1) && (verifyBlock[1] != 0x44)) {
        mountedDisk = TFS_NOT_MOUNTED;
        return TFS_NOT_MOUNTED;
    }

    // Set disk to mountedDisk variable
    mountedDisk = disk;
    return TFS_SUCCESS;
}

// Unmount 
int tfs_unmount(void) {
    if (mountedDisk < 0) {
        return TFS_NOT_MOUNTED;
    }
    
    // Close disk
    closeDisk(mountedDisk);
    mountedDisk = TFS_NOT_MOUNTED;
    return TFS_SUCCESS;
}

fileDescriptor tfs_openFile(char *name) {
    if (strlen(name) > 8) {
        return TFS_FILE_NAME_TOO_LONG;
    }

    // Initialize buffer for block building
    unsigned char buffer[BLOCKSIZE];

    // more check
    if (mountedDisk < 0) {
        return TFS_ERROR;
    }

    // Allocation
    // Search for inode
    for (int i = 1; i < numOfBlocks; i++) {
        // skip first block because it's superblock
        readBlock(mountedDisk, i, buffer); // Read blocks of data into buffer
        if (buffer[0] == 2) { // First block, type 2 or inode
            if (strncmp((char*)&buffer[4], name, 8) == 0) { // Compare Bytes 4-11 to name
                for (int j = 0; j < MAX_OPEN_FILE; j++) {
                    if (openFileTable[j].inUse != 1) {
                        openFileTable[j].inUse = 1;
                        openFileTable[j].inodeBlock = i;
                        openFileTable[j].filePointer = 0;
                        return j;
                    }
                }
                return TFS_ERROR;
            }
        }
    }

    ////////////////////////////////////
    // Before:
    // Superblock freeHead = 5
    //5 -> 8 -> 12 -> 0

    // All blocks have been checked, but no inode was found, create it
    unsigned char superBlock[BLOCKSIZE];
    unsigned char freeBlock[BLOCKSIZE];

    // Read superblock content into superblock buffer
    readBlock(mountedDisk, 0, superBlock);

    int headFreeBlock = superBlock[2]; // Inode
    if (headFreeBlock == 0) {
        return TFS_ERROR; // No free block
    }

    // Read freeblock content into freeblock buffer
    readBlock(mountedDisk, headFreeBlock, freeBlock);

    int nextFreeBlock = freeBlock[2]; // move pointer to the next freeblock
    superBlock[2] = nextFreeBlock; // now superBlock points to the next freeblock
    
    writeBlock(mountedDisk, 0, superBlock); // write back the superblock with the new ptr to the next free block

    // After:
    // Superblock freeHead = 8
    // 8 -> 12 -> 0
    ////////////////////////////////////

    // Build inode
    memset(buffer, 0, BLOCKSIZE); // Set every byte to 0 
    buffer[0] = 2;    // Set inode block
    buffer[1] = 0x44; // Magic number
    buffer[2] = 0;
    buffer[3] = 0;

    // Copy name to bytes 4-11
    strncpy((char*)&buffer[4], name, 8);

    // Update this block as inode in the mounted disk
    writeBlock(mountedDisk, headFreeBlock, buffer);
    
    // Now create open file entry 
    for (int i = 0; i < MAX_OPEN_FILE; i++) {
        if (openFileTable[i].inUse != 1) {
            openFileTable[i].inUse = 1;
            openFileTable[i].inodeBlock = headFreeBlock;
            openFileTable[i].filePointer = 0;
            return i;
        }  
    }
    
    return TFS_ERROR;
}

// Close the file
// Deallocate all system resources
// Remove table entry
int tfs_closeFile(fileDescriptor FD) {
    if (mountedDisk < 0) {
        return TFS_NOT_MOUNTED;
    }

    // Boundary check
    if (FD < 0 || FD > MAX_OPEN_FILE) {
        return TFS_ERROR;
    }

    closeDisk(FD); // Close the mounted list

    // Clear the openFileTable struct
    if (openFileTable[FD].inUse != 1) {
        return TFS_ERROR;
    } else {
        openFileTable[FD].inUse = 0;
        openFileTable[FD].inodeBlock = -1;
        openFileTable[FD].filePointer = -1;
    }

    return TFS_SUCCESS;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
    // Check mounted disk
    if (mountedDisk < 0) {
        return TFS_NOT_MOUNTED;
    }

    // Check file descriptor
    if (FD < 0 || FD >= MAX_OPEN_FILE) {
        return TFS_ERROR;
    }

    int currentInodedBlock = -1;        
    unsigned char readInodeBlock[BLOCKSIZE]; // Buffer to store inode block

    // Validate FD
    if (openFileTable[FD].inUse) {
        readBlock(mountedDisk, currentInodedBlock, readInodeBlock); // Read this inode block at this block # to buffer
        currentInodedBlock = openFileTable[FD].inodeBlock; // Get inode block
    } else {
        return TFS_ERROR;
    }

    int currentExtentBlock = readInodeBlock[INODE_FIRST_EXTENT]; // 16

    // 5 - 7 - 8 - 0
    //  Clear old data, make it free block
    while (currentExtentBlock != 0) {
        //  Record the next block extent and clear the current one
        unsigned char extentBlock[BLOCKSIZE];
        readBlock(mountedDisk, currentExtentBlock, extentBlock); //Reset the current extent block 

        int nextExtentBlock = extentBlock[2]; // Grab the next extent block

        // Free block
        ///////////////////////////////////////
        unsigned char superBlock[BLOCKSIZE];
        unsigned char freeBlock[BLOCKSIZE];
        readBlock(mountedDisk, 0, superBlock);

        int oldHead = superBlock[2];
        memset(freeBlock, 0, BLOCKSIZE);
        freeBlock[0] = 4;
        freeBlock[1] = 0x44;
        freeBlock[2] = oldHead; // Point to previous head

        writeBlock(mountedDisk, currentExtentBlock, freeBlock);
        superBlock[2] = currentExtentBlock;

        writeBlock(mountedDisk, 0, superBlock);
        ///////////////////////////////////////

        currentInodedBlock = nextExtentBlock; // Current inode block is now the nextExtentBlock
    } 

    // Now the file has no extents, allocate new blocks
    int numBlocks = ceil((double)size / 252);
    int extentBlockBuffer[numBlocks]; // Store the free extent blocks to be used to build 

    unsigned char findFreeBlock[BLOCKSIZE]; // Temp placeholder to find free blocks
    for (int i = 0; i < numBlocks; i++) {
        unsigned char superBlock[BLOCKSIZE];
        unsigned char freeBlock[BLOCKSIZE];

        // Allocating free block
        ////////////////////////////////////////////////////////////////////////////////
        readBlock(mountedDisk, 0,  superBlock); // Read super block
        int headFree = superBlock[2]; // free block head
        if (headFree == 0) {
            return TFS_ERROR; // No more free blocks
        }

        // Update the next free one to Superblock after grabbing the previous free one
        readBlock(mountedDisk, headFree, freeBlock); // Read the first free block
        int nextFree = freeBlock[2];
        superBlock[2] = nextFree;
        writeBlock(mountedDisk, 0, superBlock); // Update the disk
        ////////////////////////////////////////////////////////////////////////////////

        extentBlockBuffer[i] = headFree;
        if (extentBlockBuffer[i] < 0) {
            return TFS_ERROR;
        }
    }


    // Transfer data into extent block
    int offset = 0; // track offset
    for (int i = 0; i < numBlocks; i++) {
        unsigned char transferData[BLOCKSIZE];
        memset(transferData, 0, BLOCKSIZE); // Zero everything out

        // Building new data block
        transferData[0] = 3;
        transferData[1] = 0x44;
        if (i == numBlocks - 1) {
            transferData[2] = 0;
        } else {
            transferData[2] = extentBlockBuffer[i+1]; // Next extent block in list
        }
        
        int bytesLeftToRead = size - offset;
        int bytesToCopy = 0;
        if (bytesLeftToRead > bytesToCopy) {
            bytesToCopy = 252;
        } else {
            bytesToCopy = bytesLeftToRead;
        }

        memcpy(&transferData[4], buffer + offset, bytesToCopy); // Copy this much bytes
        writeBlock(mountedDisk, extentBlockBuffer[i], transferData); // Write to block
        offset+= bytesToCopy;
    }

    // Update inode
    if (numBlocks == 0) {
        readInodeBlock[INODE_FIRST_EXTENT] = 0;
    } else {
        readInodeBlock[INODE_FIRST_EXTENT] = extentBlockBuffer[0];
    }

    readInodeBlock[12] = size; // Update the filesize; bytes 13-15 for bigger size

    writeBlock(mountedDisk, numBlocks, readInodeBlock); // Update the inode block
    openFileTable[FD].filePointer = 0; // Update file pointer

    return TFS_SUCCESS;
}

int tfs_deleteFile(fileDescriptor FD) {
    return -1;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
    return -1;
}

int tfs_seek(fileDescriptor FD, int offset) {

    return -1;
}
