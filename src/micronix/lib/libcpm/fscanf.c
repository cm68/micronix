/*
 * fscanf - scan from stdio stream
 *
 */

#include	<stdio.h>
extern int	_doscan();

static FILE *	fpf;

static
gf()
{
	return getc(fpf);
}

static
ungf(c)
int	c;
{
	return ungetc(c, fpf);
}

fscanf(file, fmt, args)
FILE *	file;
char *	fmt;
int	args;
{
	fpf = file;
	return _doscan(gf, ungf, fmt, &args);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
