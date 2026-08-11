/*
 * lseek for micronix, which has only the v6 seek call.
 *
 * the kernel seek does not return the file position, so the
 * position of each fd is tracked in _fdpos (fdpos.s), kept
 * current by the read, write, open, creat, and dup wrappers
 * and by seek() (seek.c), which is a wrapper over lseek.
 * fds inherited across fork/exec at a nonzero offset are
 * still not tracked.
 *
 * SEEK_END takes the size from fstat, which fills in a raw
 * 36-byte v6 stat: high size byte at offset 9, low word at
 * offset 10.
 */
#include <unistd.h>

#define NFDS 16

extern long _fdpos[];
extern int errno;
extern int seekraw(unsigned char fd, int offset, int whence);

long
lseek(unsigned char fd, long offset, int whence)
{
	long pos;
	char sb[36];

	if (fd >= NFDS) {
		errno = 9;			/* EBADF */
		return -1;
	}
	switch (whence) {
	case SEEK_SET:
		pos = offset;
		break;
	case SEEK_CUR:
		pos = _fdpos[fd] + offset;
		if (offset == 0)
			return pos;	/* ftell: no seek needed */
		break;
	case SEEK_END:
		if (fstat(fd, sb) < 0)
			return -1;
		pos = ((long)(sb[9] & 0377) << 16) +
			((long)(sb[11] & 0377) << 8) +
			(sb[10] & 0377) + offset;
		break;
	default:
		errno = 22;			/* EINVAL */
		return -1;
	}
	if (pos < 0) {
		errno = 22;			/* EINVAL */
		return -1;
	}
	if (pos >> 9) {
		if (seekraw(fd, (int)(pos >> 9), 3) < 0)
			return -1;
		if ((int)pos & 511)
			if (seekraw(fd, (int)pos & 511, 1) < 0)
				return -1;
	} else {
		if (seekraw(fd, (int)pos, 0) < 0)
			return -1;
	}
	_fdpos[fd] = pos;
	return pos;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
