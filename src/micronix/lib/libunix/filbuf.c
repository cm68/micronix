/*
 * _filbuf - load up read buffer
 *
 */

extern int	read(int, void *, int);

#include	<stdio.h>
_filbuf(f)
register FILE *	f;
{
	f->_cnt = 0;
	/*
	 * fseek leaves a read-write stream with neither direction bit
	 * set: the seek is what makes the next operation free to be
	 * either one, and whichever comes first decides it.  A read has
	 * just come, so take _IOREAD here.
	 *
	 * Refusing instead made every read after a seek on a "w+"
	 * stream return EOF without issuing so much as a read - which
	 * is how asz came to write an object with its header and none
	 * of the code: it assembles into a temp file, seeks back to the
	 * start of it, and copies it into the object, and the copy read
	 * end of file from a file it had just written 62 bytes to.
	 *
	 * Both bits clear can only mean that.  fopen sets one or both,
	 * so the only stream with neither is one fseek has just left
	 * undecided; a write-only stream still has _IOWRT and is still
	 * nothing to read from.
	 */
	if (!(f->_flag & _IOREAD)) {
		if (f->_flag & _IOWRT) {
			/*
			 * A read-write stream that has been WRITING and is
			 * now being read, with no seek in between - the
			 * mirror of the case _flsbuf handles.  What is in
			 * the buffer has to reach the file before the read
			 * can see it, and after fflush the descriptor is
			 * exactly where the writer left it, so the read
			 * carries on from there.
			 *
			 * A stream that is only writable gets the refusal
			 * it deserves.
			 */
			if (!(f->_flag & _IORW))
				return(EOF);
			fflush(f);
			f->_flag &= ~_IOWRT;
		}
		f->_flag |= _IOREAD;
	}
	if (f->_base == (char *)NULL) {
		uchar	c;
		f->_cnt = 0;
		if (read(fileno(f), &c, 1) == 1)
			return(c);
		f->_flag |= _IOEOF;
		return(EOF);
	}
	if ((f->_cnt = read(fileno(f), f->_base, BUFSIZ)) <= 0) {
		if(f->_cnt == 0)
			f->_flag |= _IOEOF;
		else
			f->_flag |= _IOERR;
		return(EOF);
	}
	f->_ptr = f->_base;
	f->_cnt--;
	/*
	 * & 0377, not a cast to unsigned.  _ptr is a char *, so the
	 * byte is sign-extended to int on the way out and the cast is
	 * applied to a value that is already negative: 0x99 came back
	 * as 0xff99.  fgetc decides EOF by testing the top bit of the
	 * returned word, so every buffer whose first byte was 0x80 or
	 * over read as end of file - and a Whitesmiths object file
	 * begins with the magic 0x99, so no object could be read past
	 * its first byte.  wsld reported "read error" on files it had
	 * just opened successfully.
	 *
	 * Only the byte _filbuf itself returns was affected; the ones
	 * fgetc takes from the buffer after a refill go back through
	 * its own zero-extending path, which is why this survived
	 * everything that reads text.
	 */
	return(*f->_ptr++ & 0377);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
