/*
 * seek a stdio stream
 *
 */
#include <stdio.h>
#include <unistd.h>

int
fseek(FILE *fp, long off, int whence)
{
	if (fp->_flag & _IOERR)
		return EOF;

	if ((fp->_flag & _IOWRT) && _flsbuf(fp))
		return -1;
	else if (whence == SEEK_CUR && (fp->_flag & _IOREAD))
		off -= fp->_cnt; /* adjust for buffered but unread data */

	if (lseek(fp->_file, off, whence) < 0) {
		fp->_flag |= _IOERR;
		return EOF;
	}

	fp->_cnt = 0;
	fp->_ptr = fp->_base;

	/*
	 * Both bits, not either.  _IORW is _IOREAD|_IOWRT, so the bare
	 * "& _IORW" is true of every open stream - and stripping
	 * _IOREAD from a read-only stream made every read after a
	 * seek come back EOF.  Only a genuinely read-write stream
	 * goes back to undecided here.
	 */
	if ((fp->_flag & _IORW) == _IORW)
		fp->_flag &= ~(_IOREAD | _IOWRT);
	fp->_flag &= ~_IOEOF;

	return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
