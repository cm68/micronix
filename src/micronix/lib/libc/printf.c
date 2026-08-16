/*
 * printf to stdout
 *
 */
#include	<stdio.h>

extern int	_doprnt();

printf(f, a)
char *	f;
int	a;
{
	return(_doprnt(stdout, f, &a));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
