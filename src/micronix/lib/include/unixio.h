/*
 *	Declarations for Unix style low-level I/O functions.
 */

#ifndef	_STDDEF
typedef	int		ptrdiff_t;	/* result type of pointer difference */
typedef	unsigned	size_t;		/* type yielded by sizeof */
#define	_STDDEF
#define	offsetof(ty, mem)	((int)&(((ty *)0)->mem))
#endif	_STDDEF

#ifndef	NULL
#define	NULL	((void *)0)
#endif	NULL

extern int	errno;			/* system error number */

#ifdef notdef
/* Unix-style I/O functions */
extern int read(int fd, void *buf, int len);
extern int write(int fd, void *buf, int len);
extern int close(int fd);
extern long lseek(int fd, long offset, int whence);
#endif

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
