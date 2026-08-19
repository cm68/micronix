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

	/*
	 * _ptr - _base, not BUFSIZ - _cnt.  fflush and _flsbuf both
	 * carry this lesson already: the two agree while a stream is
	 * only ever written, and they part company after a seek, which
	 * leaves _cnt at 0 with the buffer empty.  This was the last
	 * place still trusting the subtraction - it answered a whole
	 * BUFSIZ past the truth the moment a seek had happened, so the
	 * write-back-and-return pattern (ftell the end, seek back,
	 * patch, seek to the remembered end) resumed a block too far
	 * and left a run of zeros in the file.  peep's frameless patch
	 * is what found it, by diverging from its host build; peep now
	 * counts its own bytes, and this fix is for the next caller.
	 */
	if (f->_base && (f->_flag & _IOWRT))
		return pos + (f->_ptr - f->_base);

	if (f->_cnt < 0)
		f->_cnt = 0;
	return pos - f->_cnt;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
