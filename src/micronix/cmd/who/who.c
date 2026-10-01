/*
 * who
 * who am i
 *
 * v7 who (usr/src/cmd/who.c), ported to micronix.
 *
 * cmd/who/who.c
 *
 * The listing is the one v7 wrote: name, line and time, one record to
 * a line, records with no name skipped when no argument is given.
 *
 * What the port had to say differently:
 *
 *	<utmp.h>	not used, and not for want of one.  There is an
 *			include/utmp.h in this tree and it is not the
 *			file v7's who was written against: it names the
 *			same three fields tty, name and time.  The
 *			record is defined here instead, with v7's field
 *			names, which is the way login.c defines it.  No
 *			library here reads utmp, so nothing else could
 *			have supplied it.
 *
 *	ttyname()	not in this tree's libc.  Written here the way
 *			ps.c writes one, and the way login.c's does:
 *			scan /dev, stat each entry, and answer with the
 *			one that has the device and inode fstat reports
 *			for the descriptor.  The caller wants the name
 *			after /dev/, which is what the index() below is
 *			for.
 *
 *	getpwuid()	was declared by the original and is libc's here -
 *			see <pwd.h> - so only the call remains.
 *
 * The two #ifdef interdata lines in the original printed "(Interdata)"
 * in front of the am-i answer.  This is not an Interdata, so they are
 * gone.
 *
 * And one line is added: an exit(0) at the end of main.  v7's who left
 * main by falling off the end of it, and in this tree that is not a
 * status - crt0 carries whatever the return register happens to hold,
 * so who came back 255 for a listing that had gone perfectly well.  cat
 * and the other ports here return a status; so does this.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>

struct utmp {			/* 20 bytes, v7 layout - see login.c */
	char	ut_line[8];	/* +0  the tty name, e.g. "tty2" */
	char	ut_name[8];	/* +8  the login name */
	long	ut_time;	/* +16 */
};

struct utmp utmp;
struct passwd *pw;

char *ttyname();
char *index();

main(argc, argv)
int argc;
char **argv;
{
	register char *tp, *s;
	register FILE *fi;

	s = "/etc/utmp";
	if (argc == 2)
		s = argv[1];
	if (argc == 3) {
		tp = ttyname(0);
		if (tp)
			tp = index(tp+1, '/') + 1;
		else {	/* no tty - use best guess from passwd file */
			pw = getpwuid(getuid());
			strcpy(utmp.ut_name, pw? pw->name : "?");
			strcpy(utmp.ut_line, "tty??");
			time(&utmp.ut_time);
			putline();
			exit(0);
		}
	}
	if ((fi = fopen(s, "r")) == NULL) {
		printf("who: cannot open utmp\n");
		exit(1);
	}
	while (fread((char *)&utmp, sizeof(utmp), 1, fi) == 1) {
		if (argc == 3) {
			if (strcmp(utmp.ut_line, tp))
				continue;
			putline();
			exit(0);
		}
		if (utmp.ut_name[0] == '\0' && argc == 1)
			continue;
		putline();
	}
	exit(0);
}

putline()
{
	register char *cbuf;

	printf("%-8.8s %-8.8s", utmp.ut_name, utmp.ut_line);
	cbuf = ctime(&utmp.ut_time);
	printf("%.12s\n", cbuf+4);
}

/* ------------------------------------------------------------------ *
 * ttyname - the path of the terminal open on a file descriptor.
 *
 * Not in this tree's libc.  Written the way ps.c's findtty and
 * login.c's tty_scan are written: read /dev a directory entry at a
 * time and stat each name, until one has the device and the inode that
 * fstat() reports for the descriptor.
 * ------------------------------------------------------------------ */

char *
ttyname(fd)
int fd;
{
	static char buf[32];
	struct stat dst, st;
	struct dir *dp;
	DIR *dirp;

	if (fstat(fd, &dst) < 0)
		return (0);
	if ((dirp = opendir("/dev")) == 0)
		return (0);
	while ((dp = (struct dir *)readdir(dirp)) != 0) {
		if (!dp->ino)
			continue;
		strcpy(buf, "/dev/");
		strcat(buf, dp->name);
		if (stat(buf, &st) >= 0 && st.st_dev == dst.st_dev &&
		    st.st_ino == dst.st_ino) {
			closedir(dirp);
			return (buf);
		}
	}
	closedir(dirp);
	return (0);
}
