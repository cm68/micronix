/*
 * group [-newgroup] file ...
 *
 * The group-side twin of owner (cmd/owner/owner.c), and the source
 * behind /bin/group.  There is no Berkeley original to point at - the
 * command is micronix's own - so this is written to the interface
 * group (1) documents and to the behaviour the 1982 binary has, which
 * is not quite the same thing.
 *
 * cmd/group/group.c
 *
 * With no option it reports, and with -newgroup it changes.  The
 * dash is only looked for in the FIRST argument, so "-hardware" after
 * a filename is a filename:
 *
 *	group a.out		print the group of a.out
 *	group -sales a.out	give a.out to the group sales
 *
 * The report prints the group name where /etc/group has one and the
 * number where it has not, right-justified in eight columns, then the
 * filename - "     232 /bin/ls" and "   curtg /bin/ls".  group (1)
 * describes the name-or-number and says nothing about the filename or
 * the columns; the 1982 binary prints both and this follows it.
 *
 * A group is looked up in /etc/group with getgrnam and the report
 * reads the same file the other way with getgrgid, both of which are
 * libc's - see getgrent (3).  It is always a NAME and never a number:
 * the binary answers "group -2 f" with "2: No such group", so a
 * decimal argument goes through the same lookup as any other.
 *
 * Three things here are the binary's and not the manual's, and are
 * reproduced rather than tidied because they are what a script that
 * calls this command would see:
 *
 *	the exit status	reporting a file that stats exits 255, not
 *			0 - it is the LAST file that decides, so
 *			a good file after a bad one exits 255 and a
 *			bad one after a good file exits 1.  A change
 *			exits 0.  Usage and an unknown group both
 *			exit 0, having said so on stderr.
 *
 *	the errors	they are not prefixed with the command name.
 *			A file that cannot be stat'd gives a bare
 *			perror, and an unknown group gives
 *			"name: No such group".
 *
 *	the packing	chown here takes the owner packed as
 *			uid | gid << 8 and sets both at once, and has
 *			no -1 to leave a field alone, so every change
 *			stats first and carries the owner through -
 *			the mirror of owner, which carries the group.
 *
 * One difference is not reproduced, and is not this file's to fix:
 * perror here prints "path : reason" with a space before the colon,
 * where the 1982 one printed "path: reason".  That spacing is in
 * lib/libu/perror.s and is what every command in this tree says, so
 * matching group alone would mean not using the library.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <grp.h>
#include <sys/fs.h>
#include <sys/stat.h>

struct	stat stbuf;
int	status;

main(argc, argv)
	int argc;
	char *argv[];
{
	register c;
	int gid;
	char *newgroup;
	struct group *gr;

	newgroup = NULL;
	if (argc > 1 && argv[1][0] == '-') {
		newgroup = &argv[1][1];
		argc--;
		argv++;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: group [-newgroup] file ...\n");
		exit(0);
	}

	if (newgroup == NULL) {
		for (c = 1; c < argc; c++) {
			if (stat(argv[c], &stbuf)) {
				perror(argv[c]);
				status = 1;
				continue;
			}
			gr = getgrgid(stbuf.st_gid);
			if (gr != NULL)
				printf("%8s %s\n", gr->gr_name, argv[c]);
			else
				printf("%8d %s\n", stbuf.st_gid, argv[c]);
			/*
			 * The binary's report path leaves -1 behind,
			 * not 0.  See the note at the head of the
			 * file; this is deliberate.
			 */
			status = -1;
		}
		exit(status);
	}

	/*
	 * Always a name, never a number: "group -2 f" says
	 * "2: No such group" in the 1982 binary, so a decimal
	 * argument is looked up in /etc/group like any other.
	 */
	gr = getgrnam(newgroup);
	if (gr == NULL) {
		fprintf(stderr, "%s: No such group\n", newgroup);
		exit(0);
	}
	gid = gr->gr_gid;
	for (c = 1; c < argc; c++) {
		if (stat(argv[c], &stbuf)) {
			perror(argv[c]);
			status = 1;
			continue;
		}
		if (chown(argv[c], stbuf.st_uid | (gid << 8))) {
			perror(argv[c]);
			status = 1;
			continue;
		}
		status = 0;
	}
	exit(status);
}
