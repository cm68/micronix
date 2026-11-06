/*
 * calloc - alloc space for n items of size s, and clear it to nulls
 *
 */

extern char * malloc(unsigned);
extern void	free(void *);


/* prototypes for functions defined below */
void bmove(char *, char *, int);
char *
calloc(n, s)
register int n, s;
{
	register char *cp;

	cp = malloc(n *= s);
	if(cp == (char *)0)
		return((char *)0);
	if(n) {
		*cp = 0;
		if(--n)
			bmove(cp, &cp[1], n);
	}
	return(cp);
}

cfree(s)
void *	s;
{
	free(s);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
