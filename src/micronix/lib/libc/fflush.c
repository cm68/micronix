/*
 * fflush - write buffer if dirty
 *
 */

#include	<stdio.h>

extern int	write(int, void *, int);

fflush(f)
register FILE *	f;
{
	unsigned	cnt;

	/* if nothing to flush, done */
	if (!(f->_flag & _IOWRT) || 
		f->_base == (char *)NULL || 
		(cnt = BUFSIZ - f->_cnt) == 0)
		return 0;

	if (write(fileno(f), f->_base, cnt) != cnt)
		f->_flag |= _IOERR;

	f->_cnt = BUFSIZ;
	f->_ptr = f->_base;
	if (f->_flag & _IOERR)
		return(EOF);
	return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
