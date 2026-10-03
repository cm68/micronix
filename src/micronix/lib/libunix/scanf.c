/*
 * scanf - formatted input
 *
 */

#include	<stdio.h>

extern int	_doscan();

static
gstdin()
{
	return getc(stdin);
}

static
ungstdin(c)
int	c;
{
	return ungetc(c, stdin);
}

scanf(fmt, args)
char *	fmt;
int	args;
{
	return _doscan(gstdin, ungstdin, fmt, &args);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
