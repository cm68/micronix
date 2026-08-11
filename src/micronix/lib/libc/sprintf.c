/*
 * formatted output to string
 *
 */
#include	<stdio.h>

static	FILE	spf;

sprintf(wh, f, a)
char *	wh;
char *	f;
int	a;
{
	spf._cnt = 32767;
	spf._ptr = wh;
	spf._flag = _IOWRT|_IOBINARY|_IOSTRG;
	_doprnt(&spf, f, &a);
	*spf._ptr = 0;
	return spf._ptr - wh;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
