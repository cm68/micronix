/*
 * seekraw - the 16-bit seek the compiler passes use
 *
 * On Micronix this is the bare kernel call and lseek is built on top
 * of it.  Here it is the other way round: CP/M has no seek at all,
 * the read/write pointer lives in our own fcb, and lseek is the code
 * that maintains it - so seekraw is a wrapper that narrows the offset
 * and reports success the way the callers expect.
 *
 * cpp/util.c and pass2/astio.c both reach for this to move around the
 * .n name file; they pass an int offset and want 0 or -1 back, not a
 * position.
 *
 * vim: set tabstop=4 shiftwidth=4 noexpandtab:
 */
#include	"cpm.h"

seekraw(fd, offs, whence)
uchar	fd, whence;
int	offs;
{
	if(lseek(fd, (long)offs, whence) < 0)
		return -1;
	return 0;
}
