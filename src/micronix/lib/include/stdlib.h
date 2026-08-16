/*	Standard utility functions */

#ifndef	_STDDEF
typedef	int		ptrdiff_t;	/* result type of pointer difference */
typedef	unsigned	size_t;		/* type yielded by sizeof */

#define	offsetof(ty, mem)	((int)&(((ty *)0)->mem))

#define	_STDDEF

#ifndef	NULL
#define	NULL	((void *)0)
#endif	NULL

extern int	errno;			/* system error number */
#endif	_STDDEF

#define	RAND_MAX	32767		/* max value returned by rand() */

/* no atof: there is no floating point - see math.h */
extern int	atoi(char *);
extern long	atol(char *);
extern int	rand(void);
extern void	srand(unsigned int);
extern void *	calloc(size_t, size_t);
extern void	free(void *);
extern void *	malloc(size_t);
extern void *	realloc(void *, size_t);
/*
 * malloc does not come back empty: on a machine this size there is
 * nothing useful to do with a failed allocation, and what actually
 * happened when nobody checked was that the null was written through
 * into page zero, over the rst 08 syscall trap - after which the
 * program quietly restarted itself.  malloc reports and exits.
 *
 * __malloc is the allocator underneath, and it does return 0.  It is
 * for the rare caller that can genuinely carry on without the memory
 * and wants to be told - a cache that grows until it cannot, say -
 * rather than for saving the trouble of a check.
 */
extern void *	__malloc(size_t);
extern void	abort(void);
extern void	exit(int);
extern char *	getenv(char *);
extern int	system(char *);
extern void	qsort(void *, size_t, size_t, int (*)(void *, void *));
extern int	abs(int);
extern long	labs(long);
extern int	mkstemp(char *);

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
