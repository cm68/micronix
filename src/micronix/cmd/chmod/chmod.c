/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 * chmod options mode files
 * where
 *	mode is [ugoa][+-=][rwxXstugo] or an octal number
 *	options are -Rf
 *
 * 2.11BSD chmod (chmod.c 5.5.1), ported to micronix.
 *
 * cmd/chmod/chmod.c
 *
 * What the port took out:
 *
 *	symbolic links	micronix has none, so lstat is stat and the
 *			S_IFLNK cases are gone.
 *
 *	fchdir		not a system call here, and not needed: the
 *			recursion below builds pathnames instead of
 *			changing directory, the way cp does, so there
 *			is nowhere to change back from.  The directory
 *			is closed while its subdirectories are worked
 *			on and reopened with telldir/seekdir after, so
 *			depth costs two file slots, not one per level.
 *
 *	umask		not a system call here either.  It fed the
 *			one case of "chmod +x" with no who letters,
 *			which now means a+x, as it did in v7.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>

char	*modestring, *ms;
int	status;
int	fflag;
int	rflag;
long	telldir();

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *p;
	register int i;
	struct stat st;

	if (argc < 3) {
		fprintf(stderr,
		    "Usage: chmod [-Rf] [ugoa][+-=][rwxXstugo] file ...\n");
		exit(255);
	}
	argv++, --argc;
	while (argc > 0 && argv[0][0] == '-') {
		for (p = &argv[0][1]; *p; p++) switch (*p) {

		case 'R':
			rflag++;
			break;

		case 'f':
			fflag++;
			break;

		default:
			goto done;
		}
		argc--, argv++;
	}
done:
	modestring = argv[0];
	(void) newmode(0);

	for (i = 1; i < argc; i++) {
		p = argv[i];
		if (stat(p, &st) < 0) {
			status += Perror(p);
			continue;
		}
		if (chmod(p, newmode(st.st_mode)) < 0) {
			status += Perror(p);
			continue;
		}
		if (rflag && (st.st_mode&S_IFMT) == S_IFDIR)
			status += chmodr(p);
	}
	exit(status);
}

/*
 * The mode of the directory itself is the caller's job; this walks
 * what is inside it, by full pathname.
 */
chmodr(dir)
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
		if (chmod(sub, newmode(st.st_mode)) < 0 &&
		    (ecode = Perror(sub)))
			break;
		if ((st.st_mode&S_IFMT) == S_IFDIR) {
			off = telldir(dirp);
			closedir(dirp);
			ecode = chmodr(sub);
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
		fprintf(stderr, "chmod: ");
		fprintf(stderr, fmt, a);
		putc('\n', stderr);
	}
	return (!fflag);
}

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
		fprintf(stderr, "chmod: ");
		perror(s);
	}
	return (!fflag);
}

newmode(nm)
	unsigned nm;
{
	register o, m, b;

	ms = modestring;
	m = abs();
	if (*ms == '\0')
		return (m);
	do {
		m = who();
		while (o = what()) {
			b = where(nm);
			switch (o) {
			case '+':
				nm |= b & m;
				break;
			case '-':
				nm &= ~(b & m);
				break;
			case '=':
				nm &= ~m;
				nm |= b & m;
				break;
			}
		}
	} while (*ms++ == ',');
	if (*--ms)
		fatal(255, "invalid mode");
	return (nm);
}

abs()
{
	register c, i;

	i = 0;
	while ((c = *ms++) >= '0' && c <= '7')
		i = (i << 3) + (c - '0');
	ms--;
	return (i);
}

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	01777	/* all (note absence of setuid, etc) */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */
#define	SETID	06000	/* set[ug]id */
#define	STICKY	01000	/* sticky bit */

who()
{
	register m;

	m = 0;
	for (;;) switch (*ms++) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		ms--;
		if (m == 0)
			m = ALL;
		return (m);
	}
}

what()
{

	switch (*ms) {
	case '+':
	case '-':
	case '=':
		return (*ms++);
	}
	return (0);
}

where(om)
	register om;
{
	register m;

 	m = 0;
	switch (*ms) {
	case 'u':
		m = (om & USER) >> 6;
		goto dup;
	case 'g':
		m = (om & GROUP) >> 3;
		goto dup;
	case 'o':
		m = (om & OTHER);
	dup:
		m &= (READ|WRITE|EXEC);
		m |= (m << 3) | (m << 6);
		++ms;
		return (m);
	}
	for (;;) switch (*ms++) {
	case 'r':
		m |= READ;
		continue;
	case 'w':
		m |= WRITE;
		continue;
	case 'x':
		m |= EXEC;
		continue;
	case 'X':
		if ((om & S_IFDIR) || (om & EXEC))
			m |= EXEC;
		continue;
	case 's':
		m |= SETID;
		continue;
	case 't':
		m |= STICKY;
		continue;
	default:
		ms--;
		return (m);
	}
}
