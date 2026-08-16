#include	"cpm.h"

close(fd)
uchar	fd;
{
	register struct fcb *	fc;
	uchar		luid;

	if(fd >= MAXFILE)
		return -1;
	fc = &_fcb[fd];
	luid = getuid();
	setuid(fc->uid);
	/*
	 * Tell the system how much of the last record is real before
	 * closing it.  Records are all CP/M 2 could count, so a file
	 * came back rounded up to 128 bytes and the reader had to find
	 * the end itself; CP/M 3 keeps a byte count, in s1, and this is
	 * where a program that has been writing gets to set it.  Zero
	 * means the last record is full, which is also what an empty
	 * file wants.
	 */
	if(fc->use == U_WRITE || fc->use == U_RDWR)
		fc->fil[0] = fc->rwp % SECSIZE;
	if(fc->use == U_WRITE || fc->use == U_RDWR || bdoshl(CPMVERS)&(MPM|CCPM) && fc->use == U_READ)
		bdos(CPMCLS, fc);
	fc->use = 0;
	setuid(luid);
	return 0;
}
