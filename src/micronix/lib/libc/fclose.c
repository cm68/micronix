/*
 * fclose - flush buffer if dirty, free buffer if allocated
 *
 */

#include	<stdio.h>

extern int	close(int);

fclose(f)
register FILE *	f;
{
	/*
	 * _IORW in both tests, for the two halves of the same reason.
	 *
	 * A read-write stream that has been seeked has neither
	 * direction bit - that is what fseek leaves behind so the next
	 * operation can pick one - so the first test refused to close
	 * an open file, and the second would have left _IORW set on a
	 * slot it had just released, which fopen now reads as still in
	 * use.  Closing has to undo everything opening did.
	 */
	if (!(f->_flag & (_IOREAD|_IOWRT|_IORW)))
		return(EOF);
	fflush(f);
	f->_flag &= ~(_IOREAD|_IOWRT|_IORW|_IONBF);
	if (f->_base && !(f->_flag & _IOMYBUF)) {
		_buffree(f->_base);
		f->_base = (char *)NULL;
	}
	if (close(fileno(f)) == -1 || f->_flag & _IOERR)
		return EOF;
	else
		return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
