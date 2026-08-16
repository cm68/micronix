/*
 * fopen.c - stdio fopen 
 *
 */

#include	<stdio.h>


FILE *
fopen(name, mode)
char *	name, * mode;
{
	register FILE *	f;

	/*
	 * A FREE SLOT IS ONE WITH NO DIRECTION AND NO _IORW.
	 *
	 * The test used to be the two direction bits alone, and that is
	 * exactly the state fseek leaves a read-write stream in: it
	 * clears both so the next operation can pick either, and an
	 * open "w+" file that had been seeked read as an empty slot.
	 * fopen handed it out, freopen wrote a new file over the top of
	 * it, and the fclose that followed closed a descriptor its real
	 * owner was still writing to.
	 *
	 * ld is where that landed.  It opens its output "w+b", seeks
	 * back into it to patch, then opens each input object in turn -
	 * and the first of those reopened the output's own slot.  The
	 * next write to the output came back EBADF on a descriptor
	 * nobody had knowingly closed, 31,995 bytes into the link.
	 *
	 * _IORW survives the seek, so it is what says the slot is still
	 * somebody's.
	 */
	for (f = _iob ; f != &_iob[_NFILE] ; f++)
		if (!(f->_flag & (_IOREAD|_IOWRT|_IORW)))
			break;
	if (f == &_iob[_NFILE])
		return((FILE *)NULL);
	return freopen(name, mode, f);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
