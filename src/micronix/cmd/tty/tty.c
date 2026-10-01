/*
 * tty [-s]
 *
 * v7 tty (usr/src/cmd/tty.c) and v7 ttyname (usr/src/libc/gen/ttyname.c),
 * ported to micronix.
 *
 * cmd/tty/tty.c
 *
 * tty itself is a port with nothing in it: main below is v7's, letter for
 * letter - ask ttyname for the terminal open on the standard input, print
 * the path, exit 0 if there was one and 1 if there was not, and print
 * nothing at all under -s.
 *
 * TTYNAME IS THE ONE THING THAT HAD TO MOVE INTO THE FILE.  It is in v7's
 * libc and in no library here - there is a page for it, ttyname (3),
 * recovered from the 1.3 distribution floppy, and no code behind the page -
 * so the routine travels with its only caller.  It is v7's ttyname as it
 * stands: the isatty test first, then /dev scanned for the entry whose
 * inode and device are the descriptor's, and the whole path built and
 * returned.  None of that is changed, including the check that the
 * descriptor is a character device and the cheap inode test that skips the
 * stat for most entries.
 *
 * What did change is how /dev is read.  v7 reads it a struct direct at a
 * time and that structure is <sys/dir.h>'s; this tree has no <sys/dir.h>.
 * Its directory entries are the sixteen bytes the filesystem keeps - a two
 * byte inode number and fourteen bytes of name - and cmd/login/login.c
 * already reads them that way for its own copy of this routine, so this
 * reads them the same way, and copies the name with a bound, because a name
 * of exactly fourteen characters fills the field and has no terminator
 * after it.
 *
 * The rest of the tree's difference is in what is NOT here.  login's copy
 * also counts the directory position as it goes (that is ttyslot, for
 * /etc/utmp) and hands back the bare device name, because a utmp line says
 * "ttyA"; tty has no utmp to write and wants the whole path "/dev/ttyA"
 * that the page prints, which is what v7's ttyname returns.
 *
 * The 1982 tty differs from its own page, and this port follows the page.
 * The binary prints "?" and exits 2 when it cannot name the terminal, where
 * the page and this say "not a tty" and 1, and it answers -s exactly as it
 * answers nothing.  Its syscall trace is the same scan - open("/dev/"),
 * one read of the directory, stat("/dev/ttyA") once per entry - so the
 * difference is in what it says when the scan comes up empty and nowhere
 * else.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/stat.h>

/*
 * isatty is in libu but in no header - see ttyname (3), which documents it
 * beside ttyname.  Declared here so the call below is a call and not an
 * implicit int.
 */
extern int	isatty();

/*
 * The sixteen byte directory entry /dev is read in: a two byte inode
 * number, zero for a name that has been removed, and fourteen bytes of
 * name.  The name is not always terminated - a full fourteen is all of it -
 * which is why the copy below is bounded.
 *
 * The inode number is unsigned, which matters here and not in login: an
 * inode number with its top bit set would be negative as a short, and the
 * cheap comparison below against the unsigned one in the stat buffer would
 * then never match, quietly skipping every entry above 32767.  Login only
 * ever tests this field for zero, so either type serves it.
 */
static struct {
	UINT16	de_ino;			/* +0 */
	char	de_name[14];		/* +2 */
} de;

static char	tty_path[32];		/* the answer, "/dev/" + the name */

/*
 * ttyname - the path of the terminal open on a file descriptor.
 *
 * A null return means the descriptor is not a terminal, or is one whose
 * /dev entry could not be found.  The path comes back in static storage,
 * as ttyname (3) says it does.
 */
char *
ttyname(fd)
int fd;
{
	struct stat fsb, tsb;
	int df;

	if (isatty(fd) == 0)
		return ((char *)0);
	if (fstat(fd, &fsb) < 0)
		return ((char *)0);
	if ((fsb.st_mode & S_IFMT) != S_IFCHR)
		return ((char *)0);
	if ((df = open("/dev/", 0)) < 0)
		return ((char *)0);
	while (read(df, &de, sizeof de) == sizeof de) {
		if (de.de_ino == 0)
			continue;
		if (de.de_ino != fsb.st_ino)
			continue;
		strcpy(tty_path, "/dev/");
		strncat(tty_path, de.de_name, sizeof de.de_name);
		if (stat(tty_path, &tsb) < 0)
			continue;
		if (tsb.st_dev == fsb.st_dev && tsb.st_ino == fsb.st_ino) {
			close(df);
			return (tty_path);
		}
	}
	close(df);
	return ((char *)0);
}

main(argc, argv)
int argc;
char **argv;
{
	register char *p;

	p = ttyname(0);
	if (argc == 2 && !strcmp(argv[1], "-s"))
		;
	else
		printf("%s\n", p ? p : "not a tty");
	exit(p ? 0 : 1);
}
