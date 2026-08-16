#include	"cpm.h"

dup(fd)
uchar	fd;
{
	register struct fcb *	fp;

	if(_fcb[fd].use && (fp = getfcb())) {
		/*
		 * Copy the whole fcb.  This was a struct assignment, which
		 * the compiler does not implement - it sized the copy from
		 * the byte count and fell to its two byte default, so only
		 * the drive code and the first name character came across.
		 */
		memcpy((char *)fp, (char *)&_fcb[fd], sizeof(struct fcb));
		return fp - _fcb;
	}
	return -1;
}
