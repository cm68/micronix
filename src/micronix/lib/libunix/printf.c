/*
 * printf to stdout
 *
 */
#include	<stdio.h>

extern int	_doprnt();

static
emit_stdout(c)
int	c;
{
	putc(c, stdout);
}

printf(f, a)
char *	f;
int	a;
{
	return(_doprnt(emit_stdout, f, &a));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
