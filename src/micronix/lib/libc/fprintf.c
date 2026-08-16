/*
 * formatted print to a stdio file
 *
 */
#include	<stdio.h>

extern int	_doprnt();

fprintf(file, f, a)
FILE *	file;
char *	f;
int	a;
{
	return(_doprnt(file, f, &a));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
