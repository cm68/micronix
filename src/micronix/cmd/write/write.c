/*
 * write user [ttyname]
 *
 * v7 write (usr/src/cmd/write.c), ported to micronix.
 *
 * cmd/write/write.c
 *
 * There is no write (1) page in the recovered 1982 manual - the command
 * was not documented there - so the interface is the one the source
 * gives: "usage: write user [ttyname]", a line to one named user, with
 * the second argument naming the terminal to write to when that user is
 * logged in more than once.
 *
 * What the port had to say differently:
 *
 *	<utmp.h>	not used.  The tree has an include/utmp.h whose
 *			fields are named tty, name and time; the record
 *			is defined here with v7's field names instead,
 *			the way login.c defines it, because no library
 *			here reads utmp.
 *
 *	ttyname()	not in this tree's libc.  Written here the way
 *			ps.c and login.c write one: scan /dev, stat each
 *			entry, and answer with the one whose device and
 *			inode match what fstat reports for the
 *			descriptor.
 *
 *	<signal.h>	is <sys/signal.h>, the tree's own.  The
 *			library's <signal.h> is the CP/M one and carries
 *			no SIGALRM outside #ifdef unix.
 *
 *	sigs()		the original took a function pointer, because
 *			v7's signal() did.  This tree's signal() takes
 *			the handler as a short - see lib/libu/signal.c -
 *			so sigs() takes one too, and the three calls
 *			pass eof, SIG_IGN and SIG_DFL where the original
 *			wrote eof, SIG_IGN and (int (*)())0.
 *
 *	printf(him)	printed the user's name as a format string, so a
 *			name containing a % was read as a conversion.  It
 *			is printf("%s", him) here.
 *
 * What it no longer has to say differently: the three sizes that v7
 * spelled out - the 128 byte buffer, the 32 byte histty, the eight
 * bytes of a name - are the same numbers here and are left as they
 * were written.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>
#include <sys/signal.h>

struct utmp {			/* 20 bytes, v7 layout - see login.c */
	char	ut_line[8];	/* +0  the tty name, e.g. "tty2" */
	char	ut_name[8];	/* +8  the login name */
	long	ut_time;	/* +16 */
};

struct	utmp ubuf;
int	signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
char	me[10]	= "???";
char	*him;
char	*mytty;
char	histty[32];
char	*histtya;
char	*ttyname();
char	*index();
int	logcnt;
int	eof();
int	timout();
FILE	*tf;

main(argc, argv)
int argc;
char *argv[];
{
	struct stat stbuf;
	register i;
	register FILE *uf;
	int c1, c2;

	if (argc < 2) {
		printf("usage: write user [ttyname]\n");
		exit(1);
	}
	him = argv[1];
	if (argc > 2)
		histtya = argv[2];
	if ((uf = fopen("/etc/utmp", "r")) == NULL) {
		printf("cannot open /etc/utmp\n");
		goto cont;
	}
	mytty = ttyname(2);
	if (mytty == NULL) {
		printf("Can't find your tty\n");
		exit(1);
	}
	mytty = index(mytty+1, '/') + 1;
	if (histtya) {
		strcpy(histty, "/dev/");
		strcat(histty, histtya);
	}
	while (fread((char *)&ubuf, sizeof(ubuf), 1, uf) == 1) {
		if (strcmp(ubuf.ut_line, mytty) == 0) {
			for (i = 0; i < 8; i++) {
				c1 = ubuf.ut_name[i];
				if (c1 == ' ')
					c1 = 0;
				me[i] = c1;
				if (c1 == 0)
					break;
			}
		}
		if (him[0] != '-' || him[1] != 0)
			for (i = 0; i < 8; i++) {
				c1 = him[i];
				c2 = ubuf.ut_name[i];
				if (c1 == 0)
					if (c2 == 0 || c2 == ' ')
						break;
				if (c1 != c2)
					goto nomat;
			}
		logcnt++;
		if (histty[0] == 0) {
			strcpy(histty, "/dev/");
			strcat(histty, ubuf.ut_line);
		}
	nomat:
		;
	}
cont:
	if (logcnt == 0 && histty[0] == '\0') {
		printf("%s not logged in.\n", him);
		exit(1);
	}
	fclose(uf);
	if (histtya == 0 && logcnt > 1) {
		printf("%s logged more than once\nwriting to %s\n",
			him, histty+5);
	}
	if (histty[0] == 0) {
		printf("%s", him);
		if (logcnt)
			printf(" not on that tty\n");
		else
			printf(" not logged in\n");
		exit(1);
	}
	if (access(histty, 0) < 0) {
		printf("%s: ", histty);
		printf("No such tty\n");
		exit(1);
	}
	signal(SIGALRM, timout);
	alarm(5);
	if ((tf = fopen(histty, "w")) == NULL)
		goto perm;
	alarm(0);
	if (fstat(fileno(tf), &stbuf) < 0)
		goto perm;
	if ((stbuf.st_mode & 02) == 0)
		goto perm;
	sigs(eof);
	fprintf(tf, "Message from ");
	fprintf(tf, "%s %s...\n", me, mytty);
	fflush(tf);
	for (;;) {
		char buf[128];
		i = read(0, buf, 128);
		if (i <= 0)
			eof();
		if (buf[0] == '!') {
			buf[i] = 0;
			ex(buf);
			continue;
		}
		write(fileno(tf), buf, i);
	}

perm:
	printf("Permission denied\n");
	exit(1);
}

timout()
{

	printf("Timeout opening his tty\n");
	exit(1);
}

eof()
{

	fprintf(tf, "EOF\n");
	exit(0);
}

ex(bp)
char *bp;
{
	register i;

	sigs(SIG_IGN);
	i = fork();
	if (i < 0) {
		printf("Try again\n");
		goto out;
	}
	if (i == 0) {
		sigs(SIG_DFL);
		execl("/bin/sh", "sh", "-c", bp+1, 0);
		exit(0);
	}
	while (wait((int *)NULL) != i)
		;
	printf("!\n");
out:
	sigs(eof);
}

sigs(sig)
int sig;
{
	register i;

	for (i = 0; signum[i]; i++)
		signal(signum[i], sig);
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
