/*
 *	fputc - the one-byte writer, in C
 *
 *	The Unix libc has this in assembly for speed; here it is C, which
 *	is a smaller and easier thing to trust, and the text mode below
 *	makes the routine's cost irrelevant next to the BDOS call it ends
 *	in.  Text is CP/M's default: a \n written here goes out as \r\n,
 *	and a file opened "b" (see stdio.h's _IOBINARY) skips that.
 */

#include	<stdio.h>

extern int	_flsbuf();

fputc(c, f)
int		c;
register FILE *	f;
{
	if(!(f->_flag & _IOWRT))
		return EOF;
	if((f->_flag & _IOBINARY) == 0 && c == '\n')
		fputc('\r', f);
	if(f->_cnt > 0) {
		f->_cnt--;
		*f->_ptr++ = c;
	} else
		return _flsbuf(c, f);
	return c;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
