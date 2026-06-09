#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <sys/errno.h>

#include "tinyFS.h"
#include "libDisk.h"
#include "tinyFS_errno.h"

/* -----------------------------------------------------------------------
 * Inode block layout (256 bytes)
 *   [0]      block type: 2 = inode
 *   [1]      magic: 0x44
 *   [2]      unused (next ptr, 0 for inodes)
 *   [3]      unused
 *   [4-11]   filename (8 bytes, NOT necessarily null-terminated on disk)
 *   [12-15]  file size, little-endian uint32
 *   [16]     first extent block number  (INODE_FIRST_EXTENT)
 *   [17-20]  creation timestamp, little-endian uint32  (INODE_CREATE_TIME)
 *   [21-24]  modification timestamp, little-endian uint32 (INODE_MODIFY_TIME)
 *   [25-28]  access timestamp, little-endian uint32  (INODE_ACCESS_TIME)
 *   [29-255] reserved / zero
 *
 * Extent block layout (256 bytes)
 *   [0]      block type: 3 = extent
 *   [1]      magic: 0x44
 *   [2]      next extent block number (0 = end of chain)
 *   [3]      unused
 *   [4-255]  data (252 bytes)
 *
 * Free block layout (256 bytes)
 *   [0]      block type: 4 = free
 *   [1]      magic: 0x44
 *   [2]      next free block number (0 = end of free list)
 *   [3]      unused
 *
 * Superblock layout (block 0)
 *   [0]      block type: 1 = superblock
 *   [1]      magic: 0x44
 *   [2]      head of free-block list (0 = disk full)
 *   [3]      unused
 * ----------------------------------------------------------------------- */

/* ---- helpers for 4-byte little-endian read/write ---- */
static uint32_t read_u32(const unsigned char *buf, int offset) {
    return ((uint32_t)buf[offset]) | ((uint32_t)buf[offset+1] << 8) | ((uint32_t)buf[offset+2] << 16) | ((uint32_t)buf[offset+3] << 24);
}

static void write_u32(unsigned char *buf, int offset, uint32_t val) {
    buf[offset]   = val & 0xFF;
    buf[offset+1] = (val >> 8)  & 0xFF;
    buf[offset+2] = (val >> 16) & 0xFF;
    buf[offset+3] = (val >> 24) & 0xFF;
}

/* ---- Global state ---- */
static int mountedDisk = TFS_NOT_MOUNTED;
static int numOfBlocks = 0;

typedef struct {
    int inUse;
    int inodeBlock;
    int filePointer;
} OpenTableEntry;

static OpenTableEntry openFileTable[MAX_OPEN_FILE];

/* ---- Internal: allocate one free block from the free list.
 *     Returns block number on success, -1 on failure (disk full). ---- */
static int allocFreeBlock(void) {
    unsigned char superBlock[BLOCKSIZE];
    unsigned char freeBlock[BLOCKSIZE];

    readBlock(mountedDisk, 0, superBlock);
    int head = superBlock[2];
    if (head == 0) return -1;

    readBlock(mountedDisk, head, freeBlock);
    superBlock[2] = freeBlock[2];
    writeBlock(mountedDisk, 0, superBlock);
    return head;
}

/* ---- Internal: return a block to the free list. ---- */
static void freeOneBlock(int blockNum) {
    unsigned char superBlock[BLOCKSIZE];
    unsigned char freeBlock[BLOCKSIZE];

    readBlock(mountedDisk, 0, superBlock);
    int oldHead = superBlock[2];

    memset(freeBlock, 0, BLOCKSIZE);
    freeBlock[0] = 4;
    freeBlock[1] = 0x44;
    freeBlock[2] = oldHead;
    writeBlock(mountedDisk, blockNum, freeBlock);

    superBlock[2] = blockNum;
    writeBlock(mountedDisk, 0, superBlock);
}

/* ====================================================================== */
/*  tfs_mkfs                                                               */
/* ====================================================================== */
int tfs_mkfs(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE) return TFS_ERROR;

    int usableSize = nBytes - (nBytes % BLOCKSIZE);
    int numBlocks  = usableSize / BLOCKSIZE;
    numOfBlocks    = numBlocks;

    int fd = openDisk(filename, usableSize);
    if (fd < 0) return TFS_ERROR;

    /* Superblock */
    unsigned char block[BLOCKSIZE];
    memset(block, 0, BLOCKSIZE);
    block[0] = 1;
    block[1] = 0x44;
    block[2] = 1;   /* first free block is block 1 */
    writeBlock(fd, 0, block);

    /* Free blocks: linked list 1 -> 2 -> ... -> (numBlocks-1) -> 0 */
    for (int i = 1; i < numBlocks; i++) {
        memset(block, 0, BLOCKSIZE);
        block[0] = 4;
        block[1] = 0x44;
        block[2] = (i == numBlocks - 1) ? 0 : i + 1;
        writeBlock(fd, i, block);
    }

    closeDisk(fd);
    return TFS_SUCCESS;
}

/* ====================================================================== */
/*  tfs_mount                                                              */
/* ====================================================================== */
int tfs_mount(char *diskname) {
    unsigned char verifyBlock[BLOCKSIZE];

    int disk = openDisk(diskname, 0);
    if (disk < 0) return TFS_DISK_NOT_FOUND;

    readBlock(disk, 0, verifyBlock);
    if (verifyBlock[0] != 1 || verifyBlock[1] != 0x44) {
        closeDisk(disk);
        return TFS_NOT_MOUNTED;
    }

    /* Derive numOfBlocks from actual file size */
    off_t diskSize = lseek(disk, 0, SEEK_END);
    if (diskSize <= 0) diskSize = DEFAULT_DISK_SIZE;
    numOfBlocks = (int)(diskSize / BLOCKSIZE);

    mountedDisk = disk;
    return TFS_SUCCESS;
}

/* ====================================================================== */
/*  tfs_unmount                                                            */
/* ====================================================================== */
int tfs_unmount(void) {
    if (mountedDisk < 0) return TFS_NOT_MOUNTED;

    /* Close all open files cleanly */
    for (int i = 0; i < MAX_OPEN_FILE; i++) {
        openFileTable[i].inUse = 0;
        openFileTable[i].inodeBlock = -1;
        openFileTable[i].filePointer = -1;
    }

    closeDisk(mountedDisk);
    mountedDisk = TFS_NOT_MOUNTED;
    numOfBlocks  = 0;
    return TFS_SUCCESS;
}

/* ====================================================================== */
/*  tfs_openFile                                                           */
/* ====================================================================== */
fileDescriptor tfs_openFile(char *name) {
    if (!name || strlen(name) == 0 || strlen(name) > 8)
        return TFS_FILE_NAME_TOO_LONG;
    if (mountedDisk < 0) return TFS_NOT_MOUNTED;

    unsigned char buffer[BLOCKSIZE];
    time_t now = time(NULL);

    /* Search for an existing inode with this name */
    for (int i = 1; i < numOfBlocks; i++) {
        readBlock(mountedDisk, i, buffer);
        if (buffer[0] == 2 && buffer[1] == 0x44) {
            if (strncmp((char *)&buffer[4], name, 8) == 0) {
                /* Found — update access time and open */
                write_u32(buffer, INODE_ACCESS_TIME, (uint32_t)now);
                writeBlock(mountedDisk, i, buffer);

                for (int j = 0; j < MAX_OPEN_FILE; j++) {
                    if (!openFileTable[j].inUse) {
                        openFileTable[j].inUse = 1;
                        openFileTable[j].inodeBlock = i;
                        openFileTable[j].filePointer = 0;
                        return j;
                    }
                }
                return TFS_ERROR; /* open table full */
            }
        }
    }

    /* Not found — create a new inode */
    int newBlock = allocFreeBlock();
    if (newBlock < 0) return TFS_ERROR;

    memset(buffer, 0, BLOCKSIZE);
    buffer[0] = 2;
    buffer[1] = 0x44;
    strncpy((char *)&buffer[4], name, 8);
    /* size = 0, first extent = 0 already from memset */
    write_u32(buffer, INODE_CREATE_TIME, (uint32_t)now);
    write_u32(buffer, INODE_MODIFY_TIME, (uint32_t)now);
    write_u32(buffer, INODE_ACCESS_TIME, (uint32_t)now);
    writeBlock(mountedDisk, newBlock, buffer);

    for (int j = 0; j < MAX_OPEN_FILE; j++) {
        if (!openFileTable[j].inUse) {
            openFileTable[j].inUse = 1;
            openFileTable[j].inodeBlock = newBlock;
            openFileTable[j].filePointer = 0;
            return j;
        }
    }
    /* open table full — roll back the inode we just wrote */
    freeOneBlock(newBlock);
    return TFS_ERROR;
}

/* ====================================================================== */
/*  tfs_closeFile                                                          */
/* ====================================================================== */
int tfs_closeFile(fileDescriptor FD) {
    if (mountedDisk < 0) return TFS_NOT_MOUNTED;
    if (FD < 0 || FD >= MAX_OPEN_FILE) return TFS_ERROR;
    if (!openFileTable[FD].inUse) return TFS_ERROR;

    openFileTable[FD].inUse = 0;
    openFileTable[FD].inodeBlock = -1;
    openFileTable[FD].filePointer = -1;
    return TFS_SUCCESS;
}

/* ====================================================================== */
/*  tfs_writeFile                                                          */
/* ====================================================================== */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
    if (mountedDisk < 0) return TFS_NOT_MOUNTED;
    if (FD < 0 || FD >= MAX_OPEN_FILE || !openFileTable[FD].inUse)
        return TFS_ERROR;

    int inodeBlockNum = openFileTable[FD].inodeBlock;
    unsigned char inodeBuf[BLOCKSIZE];
    readBlock(mountedDisk, inodeBlockNum, inodeBuf);

    /* Free all existing extent blocks */
    int cur = inodeBuf[INODE_FIRST_EXTENT];
    while (cur != 0) {
        unsigned char ext[BLOCKSIZE];
        readBlock(mountedDisk, cur, ext);
        int next = ext[2];
        freeOneBlock(cur);
        cur = next;
    }

    /* Allocate exactly as many new extent blocks as needed */
    int numExtents = (size == 0) ? 0 : (int)ceil((double)size / 252.0);
    int extBlocks[numExtents];

    for (int i = 0; i < numExtents; i++) {
        extBlocks[i] = allocFreeBlock();
        if (extBlocks[i] < 0) return TFS_ERROR;
    }

    /* Write data into extent blocks */
    int offset = 0;
    for (int i = 0; i < numExtents; i++) {
        unsigned char ext[BLOCKSIZE];
        memset(ext, 0, BLOCKSIZE);
        ext[0] = 3;
        ext[1] = 0x44;
        ext[2] = (i == numExtents - 1) ? 0 : extBlocks[i + 1];

        int toCopy = (size - offset > 252) ? 252 : size - offset;
        memcpy(&ext[4], buffer + offset, toCopy);
        writeBlock(mountedDisk, extBlocks[i], ext);
        offset += toCopy;
    }

    /* Update inode: size, first extent, modify + access timestamps */
    inodeBuf[INODE_FIRST_EXTENT] = (numExtents == 0) ? 0 : extBlocks[0];
    write_u32(inodeBuf, 12, (uint32_t)size);

    time_t now = time(NULL);
    write_u32(inodeBuf, INODE_MODIFY_TIME, (uint32_t)now);
    write_u32(inodeBuf, INODE_ACCESS_TIME, (uint32_t)now);

    writeBlock(mountedDisk, inodeBlockNum, inodeBuf);
    openFileTable[FD].filePointer = 0;
    return TFS_SUCCESS;
}

/* ====================================================================== */
/*  tfs_deleteFile                                                         */
/* ====================================================================== */
int tfs_deleteFile(fileDescriptor FD) {
    if (mountedDisk < 0) return TFS_NOT_MOUNTED;
    if (FD < 0 || FD >= MAX_OPEN_FILE || !openFileTable[FD].inUse)
        return TFS_ERROR;

    int inodeBlockNum = openFileTable[FD].inodeBlock;
    unsigned char inodeBuf[BLOCKSIZE];
    readBlock(mountedDisk, inodeBlockNum, inodeBuf);

    /* Free all extent blocks */
    int cur = inodeBuf[INODE_FIRST_EXTENT];
    while (cur != 0) {
        unsigned char ext[BLOCKSIZE];
        readBlock(mountedDisk, cur, ext);
        int next = ext[2];
        freeOneBlock(cur);
        cur = next;
    }

    /* Free the inode itself */
    freeOneBlock(inodeBlockNum);

    openFileTable[FD].inUse = 0;
    openFileTable[FD].inodeBlock = -1;
    openFileTable[FD].filePointer = -1;
    return TFS_SUCCESS;
}

/* ====================================================================== */
/*  tfs_readByte                                                           */
/* ====================================================================== */
int tfs_readByte(fileDescriptor FD, char *buffer) {
    if (mountedDisk < 0) return TFS_NOT_MOUNTED;
    if (FD < 0 || FD >= MAX_OPEN_FILE || !openFileTable[FD].inUse)
        return TFS_ERROR;

    unsigned char inodeBuf[BLOCKSIZE];
    readBlock(mountedDisk, openFileTable[FD].inodeBlock, inodeBuf);

    int fileSize = (int)read_u32(inodeBuf, 12);
    int fp = openFileTable[FD].filePointer;
    if (fp >= fileSize) return TFS_ERROR; /* EOF */

    int extentIdx  = fp / 252;
    int byteOffset = fp % 252;

    int nextExt = inodeBuf[INODE_FIRST_EXTENT];
    for (int i = 0; i < extentIdx; i++) {
        if (nextExt <= 0) return TFS_ERROR;
        unsigned char ext[BLOCKSIZE];
        readBlock(mountedDisk, nextExt, ext);
        nextExt = ext[2];
    }
    if (nextExt <= 0) return TFS_ERROR;

    unsigned char ext[BLOCKSIZE];
    readBlock(mountedDisk, nextExt, ext);
    *buffer = ext[4 + byteOffset];

    openFileTable[FD].filePointer++;

    /* Update access timestamp in inode */
    write_u32(inodeBuf, INODE_ACCESS_TIME, (uint32_t)time(NULL));
    writeBlock(mountedDisk, openFileTable[FD].inodeBlock, inodeBuf);

    return TFS_SUCCESS;
}

/* ====================================================================== */
/*  tfs_seek                                                               */
/* ====================================================================== */
int tfs_seek(fileDescriptor FD, int offset) {
    if (mountedDisk < 0) return TFS_NOT_MOUNTED;
    if (FD < 0 || FD >= MAX_OPEN_FILE || !openFileTable[FD].inUse)
        return TFS_ERROR;

    unsigned char inodeBuf[BLOCKSIZE];
    readBlock(mountedDisk, openFileTable[FD].inodeBlock, inodeBuf);

    int fileSize = (int)read_u32(inodeBuf, 12);
    if (offset < 0 || offset > fileSize) return TFS_ERROR;

    openFileTable[FD].filePointer = offset;
    return TFS_SUCCESS;
}

/* ====================================================================== */
/*  tfs_readdir  (extra feature b)                                        */
/*  Scans all blocks; prints name and size of every inode found.          */
/* ====================================================================== */
int tfs_readdir(void) {
    if (mountedDisk < 0) return TFS_NOT_MOUNTED;

    printf("%-10s  %s\n", "NAME", "SIZE (bytes)");
    printf("%-10s  %s\n", "----------", "------------");

    int found = 0;
    unsigned char buf[BLOCKSIZE];
    for (int i = 1; i < numOfBlocks; i++) {
        readBlock(mountedDisk, i, buf);
        if (buf[0] == 2 && buf[1] == 0x44) {
            char name[9];
            memset(name, 0, sizeof(name));
            strncpy(name, (char *)&buf[4], 8);

            int size = (int)read_u32(buf, 12);
            printf("%-10s  %d\n", name, size);
            found++;
        }
    }

    if (found == 0) printf("  (no files)\n");
    return TFS_SUCCESS;
}

/* ====================================================================== */
/*  tfs_rename  (extra feature b)                                         */
/*  Renames an open file. The file must be open (FD valid).               */
/* ====================================================================== */
int tfs_rename(fileDescriptor FD, char *newName) {
    if (mountedDisk < 0) return TFS_NOT_MOUNTED;
    if (FD < 0 || FD >= MAX_OPEN_FILE || !openFileTable[FD].inUse)
        return TFS_ERROR;
    if (!newName || strlen(newName) == 0 || strlen(newName) > 8)
        return TFS_FILE_NAME_TOO_LONG;

    /* Make sure newName is not already taken */
    unsigned char buf[BLOCKSIZE];
    for (int i = 1; i < numOfBlocks; i++) {
        readBlock(mountedDisk, i, buf);
        if (buf[0] == 2 && buf[1] == 0x44) {
            if (strncmp((char *)&buf[4], newName, 8) == 0)
                return TFS_ERROR; /* name already exists */
        }
    }

    int inodeBlockNum = openFileTable[FD].inodeBlock;
    readBlock(mountedDisk, inodeBlockNum, buf);

    memset(&buf[4], 0, 8);
    strncpy((char *)&buf[4], newName, 8);

    /* Update modification time */
    write_u32(buf, INODE_MODIFY_TIME, (uint32_t)time(NULL));

    writeBlock(mountedDisk, inodeBlockNum, buf);
    return TFS_SUCCESS;
}

/* ====================================================================== */
/*  tfs_readFileInfo  (extra feature e)                                   */
/*  Fills a FileInfo struct for the open file identified by FD.           */
/* ====================================================================== */
int tfs_readFileInfo(fileDescriptor FD, FileInfo *info) {
    if (mountedDisk < 0) return TFS_NOT_MOUNTED;
    if (FD < 0 || FD >= MAX_OPEN_FILE || !openFileTable[FD].inUse)
        return TFS_ERROR;
    if (!info) return TFS_ERROR;

    unsigned char inodeBuf[BLOCKSIZE];
    readBlock(mountedDisk, openFileTable[FD].inodeBlock, inodeBuf);

    memset(info->name, 0, sizeof(info->name));
    strncpy(info->name, (char *)&inodeBuf[4], 8);

    info->fileSize   = (int)read_u32(inodeBuf, 12);
    info->createTime = (time_t)read_u32(inodeBuf, INODE_CREATE_TIME);
    info->modifyTime = (time_t)read_u32(inodeBuf, INODE_MODIFY_TIME);
    info->accessTime = (time_t)read_u32(inodeBuf, INODE_ACCESS_TIME);

    return TFS_SUCCESS;
}