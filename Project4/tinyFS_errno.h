#ifndef TINY_FS_ERRNO_H
#define TINY_FS_ERRNO_H

#define TFS_SUCCESS 0

#define TFS_ERROR            -1
#define TFS_NOT_MOUNTED      -2
#define TFS_ALREADY_MOUNTED  -3
#define TFS_DISK_NOT_FOUND   -4
#define TFS_FILE_NAME_TOO_LONG -5
#define TFS_FILE_READ_ONLY   -6   /* write/delete on a read-only file */
#define TFS_INCONSISTENT     -7   /* consistency check failed on mount */

#endif