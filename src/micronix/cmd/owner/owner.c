/*
 * owner [-newuser] file ...
 *
 * v7 chown (usr/src/cmd/chown.c), ported to micronix.
 *
 * cmd/owner/owner.c
 *
 * The interface is the one micronix gave the command and not the one
 * the original had: owner reports an owner as well as setting one,
 * and takes the new owner behind a dash - "owner -root a.out" where
 * v7's chown took "chown root a.out".  Everything above the loop is
 * the man page; what is below it is the port.
 *
 * What the port took out:
 *
 *	chown(name, uid, gid)	chown here takes the owner packed as
 *			uid | gid << 8 and sets both at once, and has
 *			no -1 to leave a field alone.  So every change
 *			stats first and carries the group through.
 *
 * What it no longer has to say differently: getpwnam and getpwuid
 * used to be read out of /etc/passwd here, because libc had no
 * password routines at all.  They are in libc now - see
 * lib/libc/getpwent.c - so the reader that was in this file is gone
 * and the two lookups call the library.  The name that comes back is
 * the whole field rather than the sixteen bytes the old reader
 * copied, which is the one visible difference: a name longer than
 * fifteen characters was truncated here and is not any more.
 *
 * The report prints the name where /etc/passwd has one and the number
 * where it does not, a line per file in the order the files were
 * named.  The man page fixes that much and no more.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/fs.h>
#include <sys/stat.h>

struct	stat stbuf;
int	status;

main(argc, argv)
	int argc;
	char *argv[];
{
	register c;
	int uid;
	char *newuser;
	struct passwd *pw;

	newuser = NULL;
	if (argc > 1 && argv[1][0] == '-') {
		newuser = &argv[1][1];
		argc--;
		argv++;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: owner [-newuser] file ...\n");
		exit(4);
	}

	if (newuser == NULL) {
		for (c = 1; c < argc; c++) {
			if (stat(argv[c], &stbuf)) {
				Perror(argv[c]);
				continue;
			}
			pw = getpwuid(stbuf.st_uid);
			if (pw != NULL)
				printf("%s\n", pw->name);
			else
				printf("%d\n", stbuf.st_uid);
		}
		exit(status);
	}

	if (isnumber(newuser))
		uid = atoi(newuser);
	else {
		pw = getpwnam(newuser);
		if (pw == NULL)
			fatal("%s: No such user", newuser);
		uid = pw->uid;
	}
	for (c = 1; c < argc; c++) {
		if (stat(argv[c], &stbuf)) {
			Perror(argv[c]);
			continue;
		}
		if (chown(argv[c], uid | (stbuf.st_gid << 8))) {
			Perror(argv[c]);
			continue;
		}
	}
	exit(status);
}

isnumber(s)
	char *s;
{
	register int c;

	while (c = *s++)
		if (c < '0' || c > '9')
			return (0);
	return (1);
}

fatal(fmt, a)
	char *fmt, *a;
{

	fprintf(stderr, "owner: ");
	fprintf(stderr, fmt, a);
	putc('\n', stderr);
	exit(255);
}

Perror(s)
	char *s;
{

	fprintf(stderr, "owner: ");
	perror(s);
	status = 1;
	return (1);
}
