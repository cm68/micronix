#include	<stdio.h>

/*
 * The FILE table and its exit-time flush.  This is the CP/M
 * counterpart of the Unix libc's cleanup.c: where that one defines
 * _iob alongside the file descriptors, here the descriptors live in
 * _fcb (see cleanup.c in this directory) and _iob is the buffering
 * layer over them, so the two tables are separate files.
 */
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

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
