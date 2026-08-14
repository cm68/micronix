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

	/*
	 * _ptr - _base, not BUFSIZ - _cnt.  The two agree while a
	 * stream is only ever written, because fputc moves them in
	 * step, and they part company after a seek: fseek leaves _cnt
	 * at 0 and _ptr at _base, so the subtraction says a full
	 * buffer is pending when the buffer is empty, and BUFSIZ bytes
	 * of stale data go out at the seek target.  _flsbuf had the
	 * same assumption and the same fix.
	 */
	if (!(f->_flag & _IOWRT) || 
		f->_base == (char *)NULL || 
		(cnt = f->_ptr - f->_base) == 0)
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
