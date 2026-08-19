/*
 * formatted print to a stdio file
 *
 */
#include	<stdio.h>

extern int	_doprnt();

static FILE *	fpf;

static
emit_file(c)
int	c;
{
	putc(c, fpf);
}

fprintf(file, f, a)
FILE *	file;
char *	f;
int	a;
{
	fpf = file;
	return(_doprnt(emit_file, f, &a));
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
