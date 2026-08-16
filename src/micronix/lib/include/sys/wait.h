/*
 * sys/wait.h - minimal stub for process wait operations
 */

#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

/* Extract exit status from wait status */
#define WEXITSTATUS(status) (((status) >> 8) & 0xff)
#define WIFEXITED(status)   (((status) & 0x7f) == 0)

/* Stub declarations - actual implementations use system calls */
extern int waitpid(int pid, int *status, int options);
extern int wait(int *status);

#endif /* _SYS_WAIT_H */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
