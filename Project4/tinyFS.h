#ifndef TINY_FS_H
#define TINY_FS_H

/* The default size of the disk and file system block */
#define BLOCKSIZE 256

/* Your program should use a 10240 Byte disk size giving you 40 blocks
total. This is a default size. You must be able to support different
possible values */
#define DEFAULT_DISK_SIZE 10240

/* use this name for a default emulated disk file name */
#define DEFAULT_DISK_NAME "tinyFSDisk"

/* use as a special type to keep track of files */
typedef int fileDescriptor;

#define MAX_OPEN_FILE 10
#define INODE_FIRST_EXTENT 16

/* Inode timestamp byte offsets */
#define INODE_CREATE_TIME  17   /* bytes 17-20: creation timestamp (uint32_t, little-endian) */
#define INODE_MODIFY_TIME  21   /* bytes 21-24: last modification timestamp */
#define INODE_ACCESS_TIME  25   /* bytes 25-28: last access timestamp */

/* FileInfo struct returned by tfs_readFileInfo() */
typedef struct {
    char     name[9];        /* null-terminated filename (up to 8 chars) */
    int      fileSize;       /* file size in bytes */
    time_t   createTime;     /* creation time */
    time_t   modifyTime;     /* last modification time */
    time_t   accessTime;     /* last access time */
} FileInfo;

/* Core functions */
int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *diskname);
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);
int tfs_closeFile(fileDescriptor FD);
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);
int tfs_deleteFile(fileDescriptor FD);
int tfs_readByte(fileDescriptor FD, char *buffer);
int tfs_seek(fileDescriptor FD, int offset);

/* Extra feature (b): Directory listing and file renaming */
int tfs_readdir(void);
int tfs_rename(fileDescriptor FD, char *newName);

/* Extra feature (e): Timestamps */
int tfs_readFileInfo(fileDescriptor FD, FileInfo *info);

#endif