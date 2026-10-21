/*
 * open - open a file
 *
 * lib/libunix/open.c
 *
 * The program's open.  The system call underneath is sysopen()
 * (sysopen.s), which takes a name and a mode word; this wraps it with
 * the flags a caller expects from <fcntl.h>.
 *
 * O_APPEND belongs to the kernel: it has to re-seek at the moment of
 * each write, which userspace cannot do without racing.  The flag goes
 * through to the system call, and the kernel remembers it in the open
 * file's mode.
 *
 * O_CREAT and O_TRUNC are built here.  O_CREAT means trying sysopen
 * first and falling back to creat() when the name does not exist;
 * O_TRUNC means calling ftruncate() once the descriptor is open.  The
 * fallback is the one place this is not a real open: creat() opens
 * write-only, so a caller that asked for O_RDWR reopens the new file,
 * and there is a window between the two calls.  A single-user machine
 * does not race us for it, and the alternative is a kernel change.
 *
 * The third argument is the creation mode.  It is read only when
 * O_CREAT is set, which is the only time a caller must supply it.
 */

#include	<fcntl.h>
#include	<errno.h>
#include	<unistd.h>

extern int errno;
extern int sysopen();

int
open(name, flags, mode)
char *	name;
int	flags, mode;
{
	register int	fd, access;

	access = flags & 3;
	fd = sysopen(name, access | (flags & O_APPEND));
	if (fd < 0) {
		if (errno != ENOENT || !(flags & O_CREAT))
			return(-1);
		if ((fd = creat(name, mode)) < 0)
			return(-1);
		if (access != O_WRONLY || (flags & O_APPEND)) {
			close(fd);
			if ((fd = sysopen(name, access | (flags & O_APPEND))) < 0)
				return(-1);
		}
	} else if ((flags & O_CREAT) && (flags & O_EXCL)) {
		close(fd);
		errno = EEXIST;
		return(-1);
	}
	if ((flags & O_TRUNC) && access != O_RDONLY)
		ftruncate(fd, 0L);
	return(fd);
}

/* vim: tabstop=4 shiftwidth=4 noexpandtab: */
