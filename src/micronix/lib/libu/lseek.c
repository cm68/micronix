/*
 * lseek - move the read/write pointer of an open file.
 *
 * The kernel owns that pointer and reports it back, so this call no
 * longer keeps a copy of its own: a position inherited across fork or
 * exec, or shared through a dup, is the one the kernel hands over, not
 * a guess that goes stale the moment a second owner moves it.
 *
 * The kernel's seek takes its displacement as a word, so an absolute
 * position under 64K is one call.  A larger position is reached in two:
 * a block-granular absolute seek (whence 3, whose displacement the
 * kernel shifts up by 9) for the high part, then a byte-granular
 * relative seek for the remainder.  Between them they carry the whole
 * 24-bit range an inode's size field can describe.
 *
 * Before the start of the file is EINVAL, and that test belongs here:
 * seekraw() is only ever handed a non-negative displacement, so the
 * kernel's unsigned arithmetic cannot quietly wrap into a position that
 * looks legitimate.
 *
 * A refused seek leaves the pointer where it found it, which is what
 * makes SEEK_END's size query awkward: asking for the size IS a seek to
 * the end, so the position has to be read before the query, and put
 * back if the displacement is refused.
 */
#include <unistd.h>

#define CSEEK 3			/* v6's block-granular SEEK_SET */

extern int errno;
extern long seekraw(unsigned char fd, int offset, int whence);

/*
 * Move to an absolute position, in as many calls as it takes.
 */
static long
seekabs(fd, pos)
	unsigned char fd;
	unsigned long pos;
{
	if (pos <= 65535)
		return seekraw(fd, (int) pos, SEEK_SET);
	if (seekraw(fd, (int) (pos >> 9), CSEEK) < 0)
		return -1;
	if (pos & 511)
		return seekraw(fd, (int) (pos & 511), SEEK_CUR);
	return (long) pos;
}

long
lseek(unsigned char fd, long offset, int whence)
{
	unsigned long pos;	/* a file position is unsigned, up to ~15M */
	long here, save;

	switch (whence) {
	case SEEK_SET:
		if (offset < 0) {
			errno = 22;		/* EINVAL */
			return -1;
		}
		pos = (unsigned long) offset;
		break;
	case SEEK_CUR:
		/*
		 * Where we are now.  A zero displacement is ftell, and
		 * the position the query reports is the whole answer.
		 */
		here = seekraw(fd, 0, SEEK_CUR);
		if (here < 0)
			return -1;
		if (offset == 0)
			return here;
		if (offset < 0 && (unsigned long) -offset > (unsigned long) here) {
			errno = 22;		/* EINVAL: past the start */
			return -1;
		}
		pos = (unsigned long) here + (unsigned long) offset;
		break;
	case SEEK_END:
		/*
		 * The size is what a seek to the end reports, and the end
		 * is also where a zero displacement wants to land.
		 */
		if (offset == 0)
			return seekraw(fd, 0, SEEK_END);
		save = seekraw(fd, 0, SEEK_CUR);
		if (save < 0)
			return -1;
		here = seekraw(fd, 0, SEEK_END);
		if (here < 0)
			return -1;
		if (offset < 0 && (unsigned long) -offset > (unsigned long) here) {
			seekabs(fd, (unsigned long) save);
			errno = 22;		/* EINVAL: past the start */
			return -1;
		}
		pos = (unsigned long) here + (unsigned long) offset;
		break;
	default:
		errno = 22;			/* EINVAL */
		return -1;
	}
	return seekabs(fd, pos);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
