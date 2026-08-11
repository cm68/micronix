/*
 * fclose - flush buffer if dirty, free buffer if allocated
 *
 */

#include	<stdio.h>

extern int	close(int);

fclose(f)
register FILE *	f;
{
	if (!(f->_flag & (_IOREAD|_IOWRT)))
		return(EOF);
	fflush(f);
	f->_flag &= ~(_IOREAD|_IOWRT|_IONBF);
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
