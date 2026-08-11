/*
 * v6 seek, expressed on top of lseek so that the tracked
 * file position (_fdpos) stays correct no matter which
 * call a program uses.
 *
 * whence 0 and 3 displacements are unsigned, matching the
 * kernel; 1, 2, 4, 5 are signed.  whence 3-5 scale by 512.
 */
#include <unistd.h>

int
seek(unsigned char fd, int offset, int whence)
{
	long off;

	if (whence >= 3) {
		whence -= 3;
		if (whence == 0)
			off = (long) (unsigned) offset << 9;
		else
			off = (long) offset << 9;
	} else if (whence == 0) {
		off = (unsigned) offset;
	} else {
		off = offset;
	}
	if (lseek(fd, off, whence) < 0)
		return -1;
	return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
