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
/*
 * The hand-written stubs take these arguments in HL and return in HL;
 * the declarations say int so the compiler leaves them on the word
 * convention.  The functions written in C - lseek, seek - keep their
 * unsigned char fd and the A convention, consistent with their own
 * definitions.
 */
extern int access(char *pathname, int mode);
extern unsigned int alarm(unsigned int seconds);
extern int chdir(char *path);
extern int chmod(char *path, int mode);
extern int chown(char *path, int owner);
extern int close(int fd);
extern int creat(char *path, int mode);
extern int dup(int fd);
extern int dup2(int fd, int fd2);
extern int exec(char *path, char *argv[]);
extern int execve(char *path, char *argv[], char *envp[]);
extern int execv(char *path, char *argv[]);
extern int execl(char *path, ...);
extern char **	environ;
extern void exit(int status);
extern void _exit(int status);
extern int fork(void);
extern int fstat(int fd, char *buf);
extern int ftruncate(int fd, long length);
extern int getpid(void);
extern int getuid(void);
extern int gtty(int fd, char *buf);
extern int isatty(int fd);
extern int ioctl(int fd, int cmd, char *arg);
extern int kill(int pid, int sig);
extern int link(char *oldpath, char *newpath);
extern long lseek(unsigned char fd, long offset, int whence);
extern int mknod(char *path, int mode, int dev);
extern int mkdir(char *path, int mode);
extern int rename(char *from, char *to);
extern int mount(char *dev, char *dir, int flags);
extern int nice(int inc);
extern int open(char *, int, ...);
extern int pause(void);
extern int pipe(int *fds);
extern int read(int fd, char *buf, int count);
extern int seek(unsigned char fd, int offset, int whence);
extern int select(unsigned int mask);
extern int setuid(int uid);
extern void *sbrk(int incr);
extern int brk(void *addr);
extern int stat(char *path, char *buf);
extern int stime(long *tp);
extern int stty(int fd, char *buf);
extern void sync(void);
extern long time(long *tp);
extern int umount(char *target);
extern int unlink(char *pathname);
extern int wait(int *status);
extern int write(int fd, char *buf, int count);
extern int sleep(unsigned int seconds);

#endif /* _UNISTD_H */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
