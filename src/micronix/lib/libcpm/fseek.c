#include	<stdio.h>
#include	<cpm.h>

extern long	lseek(), _fsize(), ftell();

/*
 * The size of a stream, for a seek from the end.
 *
 * This used to have two answers.  A stream opened "b" was measured by
 * _fsize, the raw length; anything else was measured by reading the
 * last sector, because CP/M records a file in sectors and a text file
 * ended wherever the ctrl-Z was, which is not where the sector does.
 * The choice was made on _IOBINARY.
 *
 * There is no text mode any more - the \r\n translation came out of
 * fputc.s and fgetc.s, and the note over _IORW in stdio.h says so and
 * says nothing ever tested the flag.  fseek did, which is how it went
 * unnoticed: libcpm has not been built in either tree.  A file
 * written here is read back here, byte for byte, so the raw length is
 * the length, and the sector-reading version is gone with the flag
 * that selected it.
 */
static long
_ssize(f)
register FILE *	f;
{
	extern long _fsize();

	return _fsize(fileno(f));
}

fseek(f, offs, ptr)
register FILE *	f;
long		offs;
int		ptr;
{
	long	roffs;

	/*
	 * Clear the end-of-file flag: after a seek the stream is
	 * positioned somewhere new and whatever was true before is
	 * not any more.  This called clreof(), which does not exist
	 * anywhere in this tree - libc's fseek does the same job with
	 * the flag directly, so do that.
	 */
	f->_flag &= ~_IOEOF;
	if(!f->_base)
		if(lseek(fileno(f), offs, ptr) == -1L)
			return -1;
		else
			return 0;
	if(f->_flag & _IOWRT)
		fflush(f);
	switch(ptr) {

	case 0:	/* relative to beginning of file */
		break;

	case 1:	/* relative to current postion */
		offs += ftell(f);
		break;

	case 2:	/* relative toend of file- CP/M makes us work hard */
		offs += _ssize(f);
		break;

	default:
		return -1;
	}
	if((roffs = offs - ftell(f)) == 0)
		return 0;
	if(f->_flag & _IOREAD)
		if(roffs >= 0 && roffs <= f->_cnt) {
			f->_cnt -= roffs;
			f->_ptr += roffs;
			return 0;
		} else
			f->_cnt = 0;
	if(lseek(f->_file, offs, 0) == -1L)
		return -1;
	return 0;
}

long
ftell(f)
register FILE *	f;
{
	long	pos;

	pos = lseek(f->_file, 0L, 1);
	if(f->_cnt < 0)
		f->_cnt = 0;
	if(f->_base && f->_flag & _IOWRT)
		pos += BUFSIZ;
	return pos - f->_cnt;
}
