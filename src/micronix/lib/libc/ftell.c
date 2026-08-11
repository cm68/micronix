/*
 * ftell - return current file position
 */
#include <stdio.h>

extern long lseek();

long
ftell(f)
FILE *f;
{
	long pos;

	pos = lseek(fileno(f), 0L, 1);
	if (f->_cnt < 0)
		f->_cnt = 0;
	if (f->_base && (f->_flag & _IOWRT))
		pos += BUFSIZ;
	return pos - f->_cnt;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
