/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 * mv file1 file2
 *
 * 2.11BSD mv (mv.c 5.3.1, 1996/1/5), ported to micronix.
 *
 * What the port took out:
 *
 *	symbolic links	micronix has none; lstat is stat and the
 *			ISLNK branch is gone.
 *
 *	utimes		no call to set a time with, so the copied
 *			file's mtime is simply now.
 *
 * And what it had to say differently:
 *
 *	rename		is libu's, built from unlink and link because
 *			the kernel has no rename call.  It removes the
 *			target BEFORE trying the link, so when a
 *			cross-device link fails the target this code
 *			carefully unlinked is already gone - the
 *			fallback ignores that unlink's result instead
 *			of reporting it.
 *
 *	directories	rename(2) moved them whole; link and unlink
 *			cannot, because a directory carries its ".."
 *			and moving it to a new parent would leave that
 *			pointing at the old one, and link on a
 *			directory is the super-user's to make besides.
 *			So a directory may be renamed within its
 *			parent and nowhere else, and this says so
 *			rather than leaving the tree lying about
 *			itself.
 *
 *	devices		the device number lives in d_addr[0], and the
 *			mode stat returns carries S_ALLOC, which is
 *			masked back off before mknod repeats it.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <errno.h>

#define	MAXPATHLEN 512
#define	MAXBSIZE 512

#define	DELIM	'/'
#define MODEBITS 07777

#define	ISDIR(st)	(((st).st_mode&S_IFMT) == S_IFDIR)
#define	ISREG(st)	(((st).st_mode&S_IFMT) == S_IFREG)
#define	ISDEV(st) \
	(((st).st_mode&S_IFMT) == S_IFCHR || ((st).st_mode&S_IFMT) == S_IFBLK)

extern	int errno;
char	*dname();
struct	stat s1, s2;
/* truth values, so bytes */
char	iflag = 0;	/* interactive mode */
char	fflag = 0;	/* force overwriting */

main(argc, argv)
	register char *argv[];
{
	register i, r;
	register char *arg;
	char *dest;

	if (argc < 2)
		goto usage;
	while (argc > 1 && *argv[1] == '-') {
		argc--;
		arg = *++argv;

		/*
		 * all files following a null option
		 * are considered file names
		 */
		if (*(arg+1) == '\0')
			break;
		while (*++arg != '\0') switch (*arg) {

		case 'i':
			iflag++;
			break;

		case 'f':
			fflag++;
			break;

		default:
			goto usage;
		}
	}
	if (argc < 3)
		goto usage;
	dest = argv[argc-1];
	if (stat(dest, &s2) >= 0 && ISDIR(s2)) {
		r = 0;
		for (i = 1; i < argc-1; i++)
			r |= movewithshortname(argv[i], dest);
		exit(r);
	}
	if (argc > 3)
		goto usage;
	r = move(argv[1], argv[2]);
	exit(r);
	/*NOTREACHED*/
usage:
	fprintf(stderr,
"usage: mv [-if] f1 f2 or mv [-if] f1 ... fn d1 (`fn' is a file or directory)\n");
	return (1);
}

movewithshortname(src, dest)
	char *src, *dest;
{
	register char *shortname;
	char target[MAXPATHLEN + 1];

	shortname = dname(src);
	if (strlen(dest) + strlen(shortname) > MAXPATHLEN - 1) {
		error("%s/%s: pathname too long", dest,
			shortname);
		return (1);
	}
	sprintf(target, "%s/%s", dest, shortname);
	return (move(src, target));
}

move(source, target)
	char *source, *target;
{
	int targetexists;

	if (stat(source, &s1) < 0) {
		Perror2(source, "Cannot access");
		return (1);
	}
	/*
	 * First, try to rename source to destination.
	 * The only reason we continue on failure is if
	 * the move is on a nondirectory and not across
	 * file systems.
	 */
	targetexists = stat(target, &s2) >= 0;
	if (targetexists) {
		if (s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino) {
			error("%s and %s are identical", source, target);
			return (1);
		}
		if (iflag && !fflag && isatty(fileno(stdin)) &&
		    query("remove %s? ", target) == 0)
			return (1);
		if (access(target, 2) < 0 && !fflag && isatty(fileno(stdin))) {
			if (query("override protection %o for %s? ",
			  s2.st_mode & MODEBITS, target) == 0)
				return (1);
		}
	}
	/*
	 * A directory rename is a link and an unlink, and the link
	 * would carry the directory's ".." along unchanged - so it is
	 * only offered where ".." does not move.
	 */
	if (ISDIR(s1) && !sameparent(source, target)) {
		error("directories can only be renamed within their directory");
		return (1);
	}
	if (rename(source, target) >= 0)
		return (0);
	if (errno != EXDEV) {
		Perror2(errno == ENOENT && targetexists == 0 ? target : source,
		    "rename");
		return (1);
	}
	if (ISDIR(s1)) {
		error("can't mv directories across file systems");
		return (1);
	}
	/*
	 * rename already unlinked the target on its way to the link
	 * that failed, so there is nothing left to check here.
	 */
	if (targetexists)
		(void) unlink(target);
	/*
	 * File can't be renamed: recreate the special device, or
	 * copy the file wholesale between file systems.
	 */
	if (ISDEV(s1)) {
		if (mknod(target, s1.st_mode & ~S_ALLOC, s1.d.d_addr[0]) < 0) {
			Perror(target);
			return (1);
		}
		goto cleanup;
	}
	if (ISREG(s1)) {
		register int fi, fo, n;
		char buf[MAXBSIZE];

		fi = open(source, 0);
		if (fi < 0) {
			Perror(source);
			return (1);
		}

		fo = creat(target, s1.st_mode & MODEBITS);
		if (fo < 0) {
			Perror(target);
			close(fi);
			return (1);
		}

		for (;;) {
			n = read(fi, buf, sizeof buf);
			if (n == 0) {
				break;
			} else if (n < 0) {
				Perror2(source, "read");
				close(fi);
				close(fo);
				return (1);
			} else if (write(fo, buf, n) != n) {
				Perror2(target, "write");
				close(fi);
				close(fo);
				return (1);
			}
		}

		close(fi);
		close(fo);
		goto cleanup;
	}
	error("%s: unknown file type %o", source, s1.st_mode);
	return (1);

cleanup:
	if (unlink(source) < 0) {
		Perror2(source, "Cannot unlink");
		return (1);
	}
	return (0);
}

/*
 * do two paths name entries in the same directory?  The parents are
 * named by chopping at the last slash - "." when there is none - and
 * compared by what stat says, not by spelling, so "a" and "./x/../a"
 * come out the same when they are.
 */
sameparent(a, b)
	char *a, *b;
{
	struct stat pa, pb;
	char buf[MAXPATHLEN + 1];

	parentof(a, buf);
	if (stat(buf, &pa) < 0)
		return (0);
	parentof(b, buf);
	if (stat(buf, &pb) < 0)
		return (0);
	return (pa.st_dev == pb.st_dev && pa.st_ino == pb.st_ino);
}

parentof(path, buf)
	char *path, *buf;
{
	register char *p, *slash;

	strcpy(buf, path);
	slash = 0;
	for (p = buf; *p; p++)
		if (*p == DELIM && p[1])
			slash = p;
	if (slash == 0)
		strcpy(buf, ".");
	else if (slash == buf)
		strcpy(buf, "/");
	else
		*slash = '\0';
}

/*VARARGS*/
query(prompt, a1, a2)
	char *a1;
{
	register int i, c;

	fprintf(stderr, prompt, a1, a2);
	i = c = getchar();
	while (c != '\n' && c != EOF)
		c = getchar();
	return (i == 'y');
}

char *
dname(name)
	register char *name;
{
	register char *p;

	p = name;
	while (*p)
		if (*p++ == DELIM && *p)
			name = p;
	return name;
}

/*VARARGS*/
error(fmt, a1, a2)
	char *fmt;
{

	fprintf(stderr, "mv: ");
	fprintf(stderr, fmt, a1, a2);
	fprintf(stderr, "\n");
}

Perror(s)
	char *s;
{
	char buf[MAXPATHLEN + 10];

	sprintf(buf, "mv: %s", s);
	perror(buf);
}

Perror2(s1, s2)
	char *s1, *s2;
{
	char buf[MAXPATHLEN + 20];

	sprintf(buf, "mv: %s: %s", s1, s2);
	perror(buf);
}
