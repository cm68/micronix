/*
 * _flsbuf - worker function for fputc
 *
 */

#include	<stdio.h>

extern int	write(int, void *, int);
extern long	lseek(int, long, int);

_flsbuf(c, f)
register FILE *	f;
uchar	c;
{
	unsigned	n;

	/*
	 * fseek leaves a read-write stream with neither direction bit
	 * set: the seek is what makes the next operation free to be
	 * either one, and whichever comes first decides it.  A write
	 * has just come, so take _IOWRT here.
	 *
	 * This is the rule _filbuf already applies to reads, and says
	 * so at length.  The read side was fixed and this one was not,
	 * so a "w+" stream could be read after a seek and could not be
	 * written.  ld is what found it: it writes the image and then
	 * seeks back to patch it, and every patch failed.  That looked
	 * like a 16-bit seek offset and was nothing of the kind - it
	 * failed at offset 100 as surely as at 40000.
	 *
	 * Both bits clear can only mean an undecided stream.  fopen
	 * sets one or both, so a read-only stream still has _IOREAD
	 * and is still nothing to write to.
	 */
	if (!(f->_flag & _IOWRT)) {
		if (f->_flag & _IOREAD) {
			/*
			 * A read-write stream that has been READING and is
			 * now being written, with no seek in between.  The
			 * C standard says a program must put one there and
			 * ld does not - it patches through a window, which
			 * reads, and then writes the next segment straight
			 * out.  Doing it here costs one lseek and makes the
			 * library forgiving in the one case where it can be
			 * without guessing: the buffer holds _cnt bytes the
			 * program has not consumed, so the descriptor is
			 * that far ahead of where the program believes it
			 * is.  Put it back and the write lands where the
			 * reader had got to.
			 *
			 * A stream that is only readable gets the refusal
			 * it deserves.
			 */
			if (!(f->_flag & _IORW)) {
				f->_flag |= _IOERR;
				f->_cnt = 0;
				return(EOF);
			}
			lseek(fileno(f), -(long)f->_cnt, 1);
			f->_flag &= ~_IOREAD;
		}
		f->_flag |= _IOWRT;
		f->_ptr = f->_base;
		f->_cnt = 0;
	}

	if (f->_base == (char *)NULL) {
		f->_cnt = 0;
		if (write(fileno(f), &c, 1) == 1)
			return(c);
		f->_flag |= _IOERR;
		return(EOF);
	}

	/*
	 * WHAT IS ACTUALLY IN THE BUFFER, which is not always BUFSIZ.
	 *
	 * This wrote BUFSIZ every time, on the reasoning that fputc
	 * only calls here when the buffer is full.  That holds while a
	 * stream is only ever written and _cnt counts down from BUFSIZ
	 * in step with _ptr climbing from _base.  A seek breaks it:
	 * fseek leaves _cnt at 0 and _ptr at _base, which means
	 * "nothing buffered" to a reader and looks exactly like "full"
	 * here.  Writing BUFSIZ then puts a whole buffer of stale bytes
	 * into the file at the seek target.
	 *
	 * _ptr - _base is what is there in both cases, and is BUFSIZ in
	 * the one this used to assume.
	 */
	n = f->_ptr - f->_base;
	if (n && write(fileno(f), f->_base, n) != n)
		f->_flag |= _IOERR;

	f->_cnt = BUFSIZ-1;
	*f->_base = c;
	f->_ptr = f->_base+1;

	if (f->_flag & _IOERR)
		return(EOF);
	return(c);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
