/*
 * sscanf - formatted input from string
 *
 */

#include	<types.h>

extern int	_doscan();

static char *	sptr;

static
gstr()
{
	return *sptr ? *sptr++ : EOF;
}

static
ungstr(c)
int	c;
{
	if (c != EOF)
		--sptr;
	return c;
}

sscanf(str, fmt, args)
char *	str;
char *	fmt;
int	args;
{
	sptr = str;
	return _doscan(gstr, ungstr, fmt, &args);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
