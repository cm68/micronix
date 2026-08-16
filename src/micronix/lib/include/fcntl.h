/*
 * fcntl.h - minimal stub for file control operations
 */

#ifndef _FCNTL_H
#define _FCNTL_H

/* File access modes for open() */
#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2

/* File creation flags for open() */
#define O_CREAT     0x0100
#define O_TRUNC     0x0200
#define O_APPEND    0x0400
#define O_EXCL      0x0800

/* Stub declarations - actual implementations in unixlib.c or system calls */
extern int open(char *path, int flags);
extern int creat(char *path, int mode);

#endif /* _FCNTL_H */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
