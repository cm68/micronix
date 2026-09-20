/*
 * rmdir - remove directories
 *
 * Ported from 2.11BSD.  Micronix has no rmdir system call, so a
 * directory is removed the way rm -d does it - its "." and ".."
 * entries are unlinked, then the directory itself.  rmdir() below is
 * that part of cmd/rm.c.
 *
 * cmd/rmdir/rmdir.c
 * Changed: <2026-09-19 19:28:39 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <errno.h>

extern int errno;

main(argc, argv)
int argc;
char **argv;
{
	int errors = 0;

	if (argc < 2) {
		fprintf(stderr, "usage: %s directory ...\n", argv[0]);
		exit(1);
	}
	while (--argc)
		if (rmdir(*++argv) < 0) {
			fprintf(stderr, "rmdir: ");
			perror(*argv);
			errors++;
		}
	exit(errors != 0);
}

dotname(s)
char *s;
{
	if (s[0] == '.')
		if (s[1] == '.')
			if (s[2] == '\0')
				return (1);
			else
				return (0);
		else if (s[1] == '\0')
			return (1);
	return (0);
}

/*
 * Remove a directory: unlink its ".." entry, its "." entry, then the
 * directory itself - the three unlinks rm -d does.  There is no check
 * that the directory is empty: only the super-user can unlink a
 * directory, and, as with rm, the caller is expected to have emptied
 * it first.
 */
rmdir(f)
char *f;
{
	int status;
	char namebuf[100];

	if (dotname(f)) {
		errno = EINVAL;
		return -1;
	}
	sprintf(namebuf, "%s/..", f);
	status = unlink(namebuf);
	sprintf(namebuf, "%s/.", f);
	status += unlink(namebuf);
	status += unlink(f);
	return status;
}
