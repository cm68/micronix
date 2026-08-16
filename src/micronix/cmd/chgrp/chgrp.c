/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 * chgrp -fR gid file ...
 *
 * 2.11BSD chgrp (chgrp.c 5.7.1), ported to micronix.
 *
 * cmd/chgrp/chgrp.c
 *
 * What the port took out:
 *
 *	getgrnam and friends	no group routines in libc, so the
 *			group file is read here, the way ls reads
 *			/etc/passwd: a name is looked up in
 *			/etc/group by hand, and a number is taken as
 *			itself.
 *
 *	the membership check	it wanted getpwuid and gr_mem, and it
 *			was only advice: the kernel allows chown to
 *			the super-user and nobody else, so the call
 *			itself is the check.
 *
 *	symbolic links	micronix has none; lstat is stat.
 *
 *	fchdir		not a system call here, and not needed: the
 *			recursion builds pathnames instead of changing
 *			directory, closing the directory around each
 *			descent and seeking back after, so depth costs
 *			two file slots.
 *
 * And what it has to say differently: chown here takes the owner
 * packed as uid | gid << 8 and sets both at once, so every change
 * stats first and carries the owner through.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>

struct	stat stbuf;
int	gid;
int	status;
int	fflag, rflag;
long	telldir();

main(argc, argv)
	int argc;
	char *argv[];
{
	register c;
	register char *cp;

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

		case 'f':
			fflag++;
			break;

		case 'R':
			rflag++;
			break;

		default:
			fatal(255, "unknown option: %c", *cp);
			/*NOTREACHED*/
		}
		argv++, argc--;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: chgrp [-fR] gid file ...\n");
		exit(255);
	}
	if (isnumber(argv[0]))
		gid = atoi(argv[0]);
	else {
		gid = getgroup(argv[0]);
		if (gid < 0)
			fatal(255, "%s: unknown group", argv[0]);
	}

	for (c = 1; c < argc; c++) {
		if (stat(argv[c], &stbuf)) {
			status += Perror(argv[c]);
			continue;
		}
		if (chown(argv[c], stbuf.st_uid | (gid << 8))) {
			status += Perror(argv[c]);
			continue;
		}
		if (rflag && ((stbuf.st_mode & S_IFMT) == S_IFDIR))
			status += chgrpr(argv[c]);
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

/*
 * look a name up in /etc/group - name:passwd:gid:members - and
 * return its gid, or -1.  Character at a time, the way ls reads
 * /etc/passwd: there is no fgets in this library either.
 */
getgroup(name)
	char *name;
{
	FILE *gf;
	register int c;
	int i, j, n;
	char gname[32];

	if ((gf = fopen("/etc/group", "r")) == NULL)
		return (-1);
	for (;;) {
		i = 0;
		j = 0;
		n = 0;
		while ((c = fgetc(gf)) != '\n') {
			if (c == EOF) {
				fclose(gf);
				return (-1);
			}
			if (c == ':') {
				j++;
				continue;
			}
			if (j == 0 && i < sizeof(gname) - 1)
				gname[i++] = c;
			if (j == 2)
				n = n * 10 + c - '0';
		}
		gname[i] = '\0';
		if (strcmp(gname, name) == 0) {
			fclose(gf);
			return (n);
		}
	}
}

/*
 * The group of the directory itself is the caller's job; this walks
 * what is inside it, by full pathname.
 */
chgrpr(dir)
	char *dir;
{
	register DIR *dirp;
	register struct direct *dp;
	struct stat st;
	char sub[256];
	long off;
	int ecode;

	if ((dirp = opendir(dir)) == NULL) {
		Perror(dir);
		return (1);
	}
	ecode = 0;
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (dp->d_ino == 0)
			continue;
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (strlen(dir) + strlen(dp->d_name) + 2 > sizeof(sub)) {
			ecode = error("%s: path too long", dir);
			if (ecode)
				break;
			continue;
		}
		sprintf(sub, "%s/%s", dir, dp->d_name);
		if (stat(sub, &st) < 0) {
			ecode = Perror(sub);
			if (ecode)
				break;
			continue;
		}
		if (chown(sub, st.st_uid | (gid << 8)) < 0 &&
		    (ecode = Perror(sub)))
			break;
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			off = telldir(dirp);
			closedir(dirp);
			ecode = chgrpr(sub);
			if (ecode)
				return (ecode);
			if ((dirp = opendir(dir)) == NULL) {
				Perror(dir);
				return (1);
			}
			seekdir(dirp, off);
		}
	}
	closedir(dirp);
	return (ecode);
}

error(fmt, a)
	char *fmt, *a;
{

	if (!fflag) {
		fprintf(stderr, "chgrp: ");
		fprintf(stderr, fmt, a);
		putc('\n', stderr);
	}
	return (!fflag);
}

/* VARARGS */
fatal(status, fmt, a)
	int status;
	char *fmt, *a;
{

	fflag = 0;
	(void) error(fmt, a);
	exit(status);
}

Perror(s)
	char *s;
{

	if (!fflag) {
		fprintf(stderr, "chgrp: ");
		perror(s);
	}
	return (!fflag);
}
