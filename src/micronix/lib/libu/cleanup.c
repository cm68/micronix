/*
 * exit-time exit hook to close stdin, stdout, stderr
 *
 */
#include	<stdio.h>

_cleanup()
{
	uchar	i;
	register struct _iobuf *	ip;

	i = _NFILE;
	ip = _iob;
	do {
		fclose(ip);
		ip++;
	} while(--i);
}

/*
 * stdio file table
 */

char	_sibuf[BUFSIZ];
FILE	_iob[_NFILE] =
{
	{
		_sibuf,
		0,
		_sibuf,
		_IOREAD|_IOMYBUF,
		0			/* stdin */
	},
	{
		(char *)0,
		0,
		(char *)0,
		_IOWRT|_IONBF,
		1			/* stdout */
	},
	{
		(char *)0,
		0,
		(char *)0,
		_IOWRT|_IONBF,
		2			/* stderr */
	},
};

FILE *stdin = &_iob[0];
FILE *stdout = &_iob[1];
FILE *stderr = &_iob[2];

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
