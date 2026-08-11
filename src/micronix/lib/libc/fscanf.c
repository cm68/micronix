/*
 * fscanf - scan from stdio stream
 *
 */

#include	<stdio.h>
extern int	_doscan();

fscanf(file, fmt, args)
FILE *	file;
char *	fmt;
int	args;
{
	return _doscan(file, fmt, &args);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
