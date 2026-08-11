/*
 * fread - read from stdio file
 *
 */
#include	<stdio.h>

fread(buf, size, nitems, stream)
void *	buf;
register FILE *	stream;
unsigned	size, nitems;
{
	register char *		ptr;
	register unsigned	count;
	short	c;

	count = size * nitems;
	ptr = buf;
	while (count)
		if ((c = fgetc(stream)) == EOF)
			break;
		else {
			--count;
			*ptr++ = c;
		}
	return(nitems - (count+size-1)/size);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
