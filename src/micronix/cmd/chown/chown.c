/*
 * chown uid file ...
 *
 * v7 chown (usr/src/cmd/chown.c), ported to micronix.
 *
 * cmd/chown/chown.c
 *
 * This is the plain half of a pair.  owner (cmd/owner) is the same
 * job under the name micronix gave it, with the new owner behind a
 * dash and a mode that reports instead of setting; chown is the v7
 * interface kept alongside it - "chown root a.out", no dash, and it
 * only ever changes.
 *
 * What the port took out:
 *
 *	chown(name, uid, gid)	chown here takes the owner packed as
 *			uid | gid << 8 and sets both at once, and has
 *			no -1 to leave a field alone.  So every change
 *			stats first and carries the group through,
 *			exactly as owner does.
 *
 *	getpwnam	v7's is libc's now - see getpwent (3) - which
 *			is why this port waited for that one.
 *
 * And what it had to say differently, both being the tree's rules
 * rather than v7's:
 *
 *	stat		the original called stat and did not look at
 *			the answer, so a file that could not be stat'd
 *			was chowned with whatever st_gid happened to
 *			hold - the group would silently become 0.
 *			Here a failed stat is reported and the file
 *			skipped.
 *
 *	stderr		the original said everything through printf,
 *			usage and errors alike.  Errors go to stderr
 *			here, as they do in owner.
 *
 * There is no /bin/chown to match - micronix shipped owner instead -
 * so nothing here is fixed by a binary, only by the original source
 * and by owner.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/fs.h>
#include <sys/stat.h>

struct	stat stbuf;
int	uid;
int	status;

main(argc, argv)
	int argc;
	char *argv[];
{
	register c;
	struct passwd *pwd;

	if (argc < 3) {
		fprintf(stderr, "usage: chown uid file ...\n");
		exit(4);
	}
	if (isnumber(argv[1]))
		uid = atoi(argv[1]);
	else {
		if ((pwd = getpwnam(argv[1])) == NULL) {
			fprintf(stderr, "chown: unknown user id: %s\n",
				argv[1]);
			exit(4);
		}
		uid = pwd->uid;
	}
	for (c = 2; c < argc; c++) {
		if (stat(argv[c], &stbuf)) {
			perror(argv[c]);
			status = 1;
			continue;
		}
		if (chown(argv[c], uid | (stbuf.st_gid << 8))) {
			perror(argv[c]);
			status = 1;
			continue;
		}
	}
	exit(status);
}

isnumber(s)
	char *s;
{
	register c;

	while (c = *s++)
		if (c < '0' || c > '9')
			return (0);
	return (1);
}
