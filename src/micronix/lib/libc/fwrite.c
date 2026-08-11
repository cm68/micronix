/*
 * write to stdio stream
 *
 */
#include	<stdio.h>

fwrite(buf, size, nitems, stream)
void *	buf;
register FILE *	stream;
unsigned	size, nitems;
{
	register unsigned	count;
	register char *		ptr;

	count = size * nitems;
	ptr = buf;
	while (count)
		if (putc(*ptr++, stream) == EOF)
			break;
		else
			--count;
	return(nitems - (count+size-1)/size);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
