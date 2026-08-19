#include	<stdio.h>

extern char *	sbrk();

/*
 * The stdio buffer pool.  fopen hands a FILE its 512 byte buffer from
 * here, and fclose hands it back, so a program that opens and closes
 * files in sequence does not leak a buffer each time.
 */
static union stdbuf
{
	char		bufarea[BUFSIZ];
	union stdbuf *	link;
} *	freep;

char *
_bufallo()
{
	register union stdbuf *	pp;

	if(pp = freep)
		freep = pp->link;
	else
		pp = (union stdbuf *)sbrk(BUFSIZ);
	return pp->bufarea;
}

_buffree(pp)
char *	pp;
{
	register union stdbuf *	up;

	up = (union stdbuf *)pp;
	up->link = freep;
	freep = up;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
