/*
 * routines for getchar and putchar, usually macros
 *
 */

#include	<stdio.h>

#undef	getchar
#undef	putchar

getchar()
{
	return(fgetc(stdin));
}

putchar(c)
{
	return(fputc(c, stdout));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
