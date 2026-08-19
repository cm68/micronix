/*
 *	Real functions behind the getchar/putchar macros
 */

#include	<stdio.h>

#undef	getchar
#undef	putchar
getchar()
{
	return(getc(stdin));
}

putchar(c)
{
	return(putc(c, stdout));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
