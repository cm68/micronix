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

	/*
	 * fflush, not _flsbuf.  _flsbuf is fputc's worker and takes two
	 * arguments - the character and the stream - so calling it with
	 * one passed the FILE * as the character and read the stream
	 * pointer off whatever was next on the stack.  It returned EOF,
	 * and fseek gave up here without ever issuing the lseek.
	 *
	 * It would have been the wrong call even with the right
	 * arguments: _flsbuf writes a whole BUFSIZ, because the only
	 * time fputc calls it the buffer is full.  fflush writes the
	 * BUFSIZ - _cnt bytes that are actually there.
	 *
	 * asz is what found it.  It assembles into a temp file it has
	 * already unlinked, then seeks back to the start and copies it
	 * into the object.  The seek silently did nothing, the copy
	 * read end-of-file, and the object came out as a bare sixteen
	 * byte header - the assembled code went to the temp at fclose,
	 * by which time it had no name left to be found under.
	 */
	if ((fp->_flag & _IOWRT) && fflush(fp))
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
