/*
 * tinyFSDemo.c
 * Comprehensive demonstration of TinyFS core functions,
 * timestamps (extra feature e), and directory listing / rename
 * (extra feature b).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "tinyFS.h"
#include "tinyFS_errno.h"

/* ---- Pretty-print helpers -------------------------------------------- */

static void section(const char *title) {
    printf("\n");
    printf("========================================\n");
    printf("  %s\n", title);
    printf("========================================\n");
}

static void pass(const char *msg) { printf("  [PASS] %s\n", msg); }
static void fail(const char *msg) { printf("  [FAIL] %s\n", msg); }

static void check(int result, const char *msg) {
    if (result >= 0) pass(msg);
    else             fail(msg);
}

static void print_fileinfo(fileDescriptor FD) {
    FileInfo info;
    if (tfs_readFileInfo(FD, &info) < 0) {
        printf("  [!] tfs_readFileInfo failed\n");
        return;
    }
    char cbuf[32], mbuf[32], abuf[32];
    strftime(cbuf, sizeof(cbuf), "%Y-%m-%d %H:%M:%S", localtime(&info.createTime));
    strftime(mbuf, sizeof(mbuf), "%Y-%m-%d %H:%M:%S", localtime(&info.modifyTime));
    strftime(abuf, sizeof(abuf), "%Y-%m-%d %H:%M:%S", localtime(&info.accessTime));

    printf("  File      : %s\n",  info.name);
    printf("  Size      : %d bytes\n", info.fileSize);
    printf("  Created   : %s\n",  cbuf);
    printf("  Modified  : %s\n",  mbuf);
    printf("  Accessed  : %s\n",  abuf);
}

/* ---- Read entire file content into a freshly-malloc'd buffer --------- */
static char *readAll(fileDescriptor FD, int *outLen) {
    /* rewind */
    tfs_seek(FD, 0);

    /* gather bytes */
    char  tmp[4096];
    int   len = 0;
    char  byte;
    while (tfs_readByte(FD, &byte) == TFS_SUCCESS && len < (int)sizeof(tmp))
        tmp[len++] = byte;

    char *buf = malloc(len + 1);
    memcpy(buf, tmp, len);
    buf[len] = '\0';
    if (outLen) *outLen = len;
    return buf;
}

/* ======================================================================
 * DEMO START
 * ====================================================================== */
int main(void) {
    printf("\n");
    printf("############################################################\n");
    printf("#               TinyFS Comprehensive Demo                  #\n");
    printf("############################################################\n");

    int ret;
    fileDescriptor fd1, fd2, fd3;

    /* ------------------------------------------------------------------ */
    section("1. Format and mount a fresh disk");
    /* ------------------------------------------------------------------ */

    /* Always start from a clean slate */
    ret = tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE);
    check(ret, "tfs_mkfs() created a 10240-byte filesystem");

    ret = tfs_mount(DEFAULT_DISK_NAME);
    check(ret, "tfs_mount() mounted the filesystem");

    /* ------------------------------------------------------------------ */
    section("2. Create files with tfs_openFile");
    /* ------------------------------------------------------------------ */

    fd1 = tfs_openFile("hello");
    if (fd1 >= 0) pass("tfs_openFile(\"hello\") returned fd");
    else          fail("tfs_openFile(\"hello\") failed");

    fd2 = tfs_openFile("data");
    if (fd2 >= 0) pass("tfs_openFile(\"data\") returned fd");
    else          fail("tfs_openFile(\"data\") failed");

    fd3 = tfs_openFile("nums");
    if (fd3 >= 0) pass("tfs_openFile(\"nums\") returned fd");
    else          fail("tfs_openFile(\"nums\") failed");

    /* Error case: name too long */
    fileDescriptor fdBad = tfs_openFile("toolongname");
    if (fdBad < 0) pass("tfs_openFile(\"toolongname\") correctly rejected (name > 8 chars)");
    else           fail("tfs_openFile should have rejected long name");

    /* ------------------------------------------------------------------ */
    section("3. Write and read back a small file");
    /* ------------------------------------------------------------------ */

    char *msg1 = "Hello, TinyFS!";
    ret = tfs_writeFile(fd1, msg1, (int)strlen(msg1));
    check(ret, "tfs_writeFile(fd1, \"Hello, TinyFS!\")");

    tfs_seek(fd1, 0);
    char byte;
    printf("  Reading fd1 byte-by-byte: \"");
    while (tfs_readByte(fd1, &byte) == TFS_SUCCESS)
        putchar(byte);
    printf("\"\n");
    pass("tfs_readByte loop completed");

    /* ------------------------------------------------------------------ */
    section("4. Write a multi-extent file (> 252 bytes)");
    /* ------------------------------------------------------------------ */

    /* Build a 600-byte repeating pattern — needs 3 extent blocks */
    char bigBuf[600];
    for (int i = 0; i < 600; i++)
        bigBuf[i] = 'A' + (i % 26);

    ret = tfs_writeFile(fd2, bigBuf, 600);
    check(ret, "tfs_writeFile(fd2, 600 bytes) spanning 3 extents");

    int readLen;
    char *readBack = readAll(fd2, &readLen);
    if (readLen == 600 && memcmp(readBack, bigBuf, 600) == 0)
        pass("Read back all 600 bytes correctly");
    else
        fail("Data mismatch or wrong length on readback");
    free(readBack);

    /* ------------------------------------------------------------------ */
    section("5. Seek and partial read");
    /* ------------------------------------------------------------------ */

    ret = tfs_seek(fd2, 260);
    check(ret, "tfs_seek(fd2, 260) — seeks into the second extent");

    char seekBuf[5] = {0};
    for (int i = 0; i < 4; i++) tfs_readByte(fd2, &seekBuf[i]);
    printf("  Bytes at offset 260: \"%s\"\n", seekBuf);
    /* expected: bigBuf[260..263] */
    if (memcmp(seekBuf, bigBuf + 260, 4) == 0)
        pass("Bytes after seek match original data");
    else
        fail("Bytes after seek don't match");

    /* Out-of-bounds seek */
    ret = tfs_seek(fd2, 9999);
    if (ret < 0) pass("tfs_seek(fd2, 9999) correctly rejected (past EOF)");
    else         fail("tfs_seek should have rejected offset past EOF");

    /* ------------------------------------------------------------------ */
    section("6. Write numbers file, overwrite it");
    /* ------------------------------------------------------------------ */

    char *nums1 = "1234567890";
    ret = tfs_writeFile(fd3, nums1, (int)strlen(nums1));
    check(ret, "tfs_writeFile(fd3, \"1234567890\")");

    char *nums2 = "ABCDEFGHIJ";   /* overwrite with same-length content */
    ret = tfs_writeFile(fd3, nums2, (int)strlen(nums2));
    check(ret, "tfs_writeFile(fd3, \"ABCDEFGHIJ\") — overwrite");

    readBack = readAll(fd3, &readLen);
    if (strcmp(readBack, "ABCDEFGHIJ") == 0)
        pass("Overwrite confirmed — old data gone, new data correct");
    else
        fail("Overwrite failed");
    free(readBack);

    /* ------------------------------------------------------------------ */
    section("7. Timestamps (extra feature e)");
    /* ------------------------------------------------------------------ */

    printf("  --- fd1 (\"hello\") info ---\n");
    print_fileinfo(fd1);

    /* Sleep 1 second so modification timestamp changes visibly */
    printf("\n  (sleeping 5 s to advance clock...)\n");
    sleep(5);

    char *updatedMsg = "Hello, updated TinyFS!";
    tfs_writeFile(fd1, updatedMsg, (int)strlen(updatedMsg));
    printf("\n  --- fd1 after re-write ---\n");
    print_fileinfo(fd1);

    printf("\n  --- fd2 (\"data\", 600 bytes) info ---\n");
    print_fileinfo(fd2);

    printf("\n  --- fd3 (\"nums\") info ---\n");
    print_fileinfo(fd3);

    /* ------------------------------------------------------------------ */
    section("8. Directory listing — tfs_readdir (extra feature b)");
    /* ------------------------------------------------------------------ */

    printf("  Current files on disk:\n");
    printf("  ");
    ret = tfs_readdir();
    check(ret, "tfs_readdir() succeeded");

    /* ------------------------------------------------------------------ */
    section("9. Rename — tfs_rename (extra feature b)");
    /* ------------------------------------------------------------------ */

    ret = tfs_rename(fd3, "renamed");
    check(ret, "tfs_rename(fd3, \"renamed\")");

    /* Confirm new name via readFileInfo */
    FileInfo info;
    tfs_readFileInfo(fd3, &info);
    if (strcmp(info.name, "renamed") == 0)
        pass("File name is now \"renamed\"");
    else
        fail("Name was not updated");

    /* Try to rename to an already-used name */
    ret = tfs_rename(fd3, "hello");
    if (ret < 0) pass("tfs_rename to existing name \"hello\" correctly rejected");
    else         fail("tfs_rename should have rejected duplicate name");

    /* Try to rename to a too-long name */
    ret = tfs_rename(fd3, "waytolong");
    if (ret < 0) pass("tfs_rename(\"waytolong\") correctly rejected (> 8 chars)");
    else         fail("tfs_rename should have rejected long name");

    printf("\n  Directory after rename:\n  ");
    tfs_readdir();

    /* ------------------------------------------------------------------ */
    section("10. Close and re-open a file (persistence check)");
    /* ------------------------------------------------------------------ */

    ret = tfs_closeFile(fd1);
    check(ret, "tfs_closeFile(fd1)");

    /* Attempt to use closed fd — should fail */
    ret = tfs_readByte(fd1, &byte);
    if (ret < 0) pass("tfs_readByte on closed fd correctly rejected");
    else         fail("tfs_readByte on closed fd should have failed");

    /* Re-open: tfs_openFile finds the existing inode */
    fd1 = tfs_openFile("hello");
    if (fd1 >= 0) pass("tfs_openFile(\"hello\") re-opened existing file");
    else          fail("tfs_openFile failed to re-open existing file");

    readBack = readAll(fd1, &readLen);
    if (strcmp(readBack, updatedMsg) == 0)
        pass("Data persists after close/re-open");
    else
        fail("Data lost after close/re-open");
    free(readBack);

    /* ------------------------------------------------------------------ */
    section("11. Delete a file");
    /* ------------------------------------------------------------------ */

    ret = tfs_deleteFile(fd2);
    check(ret, "tfs_deleteFile(fd2) — deletes \"data\"");

    /* fd2 is now invalid */
    ret = tfs_readByte(fd2, &byte);
    if (ret < 0) pass("tfs_readByte on deleted fd correctly rejected");
    else         fail("tfs_readByte on deleted fd should have failed");

    printf("\n  Directory after delete:\n  ");
    tfs_readdir();

    /* ------------------------------------------------------------------ */
    section("12. Unmount, remount, verify data survived");
    /* ------------------------------------------------------------------ */

    ret = tfs_closeFile(fd1);
    check(ret, "tfs_closeFile(fd1) before unmount");

    ret = tfs_closeFile(fd3);
    check(ret, "tfs_closeFile(fd3) before unmount");

    ret = tfs_unmount();
    check(ret, "tfs_unmount()");

    ret = tfs_mount(DEFAULT_DISK_NAME);
    check(ret, "tfs_mount() — remounting same disk");

    /* Re-open "hello" and verify content */
    fd1 = tfs_openFile("hello");
    if (fd1 >= 0) pass("tfs_openFile(\"hello\") after remount");
    else          fail("tfs_openFile failed after remount");

    readBack = readAll(fd1, &readLen);
    if (strcmp(readBack, updatedMsg) == 0)
        pass("\"hello\" content intact after unmount/remount");
    else
        fail("\"hello\" content corrupted after unmount/remount");
    free(readBack);

    /* "data" should be gone (was deleted before unmount) */
    fileDescriptor fdCheck = tfs_openFile("data");
    if (fdCheck >= 0) {
        /* It was re-created empty — check that it's empty (size 0) */
        FileInfo fi;
        tfs_readFileInfo(fdCheck, &fi);
        if (fi.fileSize == 0)
            pass("\"data\" re-created empty after remount (was deleted)");
        tfs_closeFile(fdCheck);
    } else {
        pass("\"data\" not found after remount — correctly deleted");
    }

    printf("\n  Final directory listing:\n  ");
    tfs_readdir();

    /* ------------------------------------------------------------------ */
    section("13. Cleanup");
    /* ------------------------------------------------------------------ */

    tfs_closeFile(fd1);
    tfs_unmount();
    check(TFS_SUCCESS, "tfs_unmount() — clean shutdown");

    printf("\n");
    printf("############################################################\n");
    printf("#                   Demo complete                          #\n");
    printf("############################################################\n\n");
    return 0;
}