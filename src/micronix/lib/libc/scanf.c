/*
 * scanf - formatted input
 *
 */

#include	<stdio.h>

extern int	_doscan();

scanf(fmt, args)
char *	fmt;
int	args;
{
	return _doscan(stdin, fmt, &args);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
