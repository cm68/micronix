/*
 * unistd.h - POSIX system calls for Micronix
 */

#ifndef _UNISTD_H
#define _UNISTD_H

/* Standard file descriptors */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* access() mode flags */
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

/* seek() whence values - stdio.h has these too, and C puts them there */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/* System call prototypes */
extern char access(char *pathname, unsigned char mode);
extern unsigned int alarm(unsigned int seconds);
extern int chdir(char *path);
extern int chmod(char *path, int mode);
extern int chown(char *path, int owner);
extern int close(unsigned char fd);
extern int creat(char *path, int mode);
extern int dup(unsigned char fd);
extern int exec(char *path, char *argv[]);
extern int execv(char *path, char *argv[]);
extern void exit(int status);
extern int fork(void);
extern int fstat(unsigned char fd, char *buf);
extern int getpid(void);
extern int getuid(void);
extern int gtty(unsigned char fd, char *buf);
extern int kill(int pid, int sig);
extern int link(char *oldpath, char *newpath);
extern long lseek(unsigned char fd, long offset, int whence);
extern int mknod(char *path, int mode, int dev);
extern int mount(char *dev, char *dir, int flags);
extern int nice(int inc);
extern int open(char *path, int flags);
extern int pause(void);
extern int pipe(int *fds);
extern int read(unsigned char fd, char *buf, int count);
extern long lseek(unsigned char fd, long offset, int whence);
extern int seek(unsigned char fd, int offset, int whence);
extern int setuid(int uid);
extern void *sbrk(int incr);
extern int brk(void *addr);
extern int stat(char *path, char *buf);
extern int stime(long *tp);
extern int stty(unsigned char fd, char *buf);
extern void sync(void);
extern int time(long *tp);
extern int umount(char *target);
extern int unlink(char *pathname);
extern int wait(int *status);
extern int write(unsigned char fd, char *buf, int count);
extern int sleep(unsigned int seconds);

#endif /* _UNISTD_H */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
