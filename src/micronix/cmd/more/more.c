/*
 * more - page output on a terminal, including output arriving in a
 * pipe, which is the whole reason it exists here.
 *
 * cmd/more/more.c
 *
 * The more that came with the system is a flat binary with no source,
 * and after every --More-- it reads its keystroke from FD 0.  With a
 * file argument that is the terminal and it pages; in "man sh | more"
 * it is the pipe, so it eats half a kilobyte of the very text it was
 * supposed to be holding back and sails on.  It never asks gtty
 * whether anything is a terminal at all.
 *
 * This one asks.  The keystroke fd is standard input when that is a
 * terminal, and standard error when it is not - in a pipeline the
 * write side of 2 still leads to the keyboard's terminal, which is
 * the same arrangement more has used since it learned to sit in
 * pipelines.  If standard output is not a terminal there is nobody
 * to page for, and it copies its input through untouched.
 *
 * The keystroke itself is read in RAW mode without echo, put on for
 * just that one read and taken off again, so the page text always
 * goes out through a cooked terminal.  A ^C arriving during the read
 * comes back as a character rather than a signal - RAW mode holds
 * the signals too - and quits, the same as q.
 *
 * space is the next page, return the next line, q the end.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/sgtty.h>

#define	LINES	24
#define	COLS	80

int	plines = LINES - 1;			/* text lines per screenful */
char	kfd = -1;					/* the keystroke fd, -1 = no paging */
int	left;						/* lines before the next prompt */
char	col;						/* output column, 0..COLS */
char	obuf[512];
int	ocnt;

char	prompt[] = "--More--";
char	eraser[] = "\r        \r";

main(argc, argv)
int	argc;
char	*argv[];
{
	char junk[6];
	register int i;
	int fd, many;

	argc--;
	argv++;
	if (argc > 0 && argv[0][0] == '-') {
		i = atoi(&argv[0][1]);
		if (i > 0)
			plines = i;
		argc--;
		argv++;
	}

	/*
	 * paging happens when the output is a terminal, and the keys
	 * come from input if input is one, from error otherwise.
	 */
	if (gtty(1, junk) == 0) {
		if (gtty(0, junk) == 0)
			kfd = 0;
		else if (gtty(2, junk) == 0)
			kfd = 2;
	}
	left = plines;

	if (argc == 0) {
		page(0);
	} else {
		many = argc > 1;
		for (i = 0; i < argc; i++) {
			if ((fd = open(argv[i], 0)) < 0) {
				oflush();
				fprintf(stderr, "more: cannot open %s\n", argv[i]);
				continue;
			}
			if (many)
				banner(argv[i]);
			page(fd);
			close(fd);
		}
	}
	oflush();
	exit(0);
}

/*
 * the header between files, the shape v7 gave it
 */
banner(name)
char	*name;
{
	outstr("::::::::::::::\n");
	outstr(name);
	outstr("\n::::::::::::::\n");
}

/*
 * copy fd to standard output, stopping for a keystroke every
 * screenful.  With no keystroke fd it is cat.
 */
page(fd)
int	fd;
{
	char buf[512];
	register char *p;
	register int n;

	while ((n = read(fd, buf, sizeof buf)) > 0) {
		for (p = buf; n > 0; n--, p++) {
			if (kfd >= 0 && left <= 0) {
				oflush();
				write(1, prompt, sizeof prompt - 1);
				if (ask() == 0)
					done();
				write(1, eraser, sizeof eraser - 1);
			}
			outc(*p);
			if (*p == '\n') {
				left--;
				col = 0;
			} else if (++col >= COLS) {
				/*
				 * the terminal wrapped this line for
				 * us; it spent a screen line doing it
				 */
				left--;
				col = 0;
			}
		}
	}
}

/*
 * one keystroke, raw and unechoed for exactly the read.  space is a
 * screenful, return a line, q - or the ^C that raw mode turns into a
 * character - the end.  Anything else is a screenful too; this pager
 * does not have v7's vocabulary and does not pretend to.
 */
ask()
{
	struct sgtty omode, rmode;
	char c;
	register int n;

	gtty(kfd, &omode);
	gtty(kfd, &rmode);
	rmode.mode |= RAW;
	rmode.mode &= ~ECHO;
	stty(kfd, &rmode);
	n = read(kfd, &c, 1);
	stty(kfd, &omode);

	if (n <= 0 || c == 'q' || c == 'Q' || c == 003)
		return (0);
	if (c == '\r' || c == '\n')
		left = 1;
	else
		left = plines;
	return (1);
}

done()
{
	write(1, "\n", 1);
	exit(0);
}

/*
 * output is gathered so that a screenful is a handful of writes and
 * not two thousand: on this machine every write is a system call
 * through the emulator, and the difference is visible at the
 * terminal.
 */
outc(c)
char	c;
{
	obuf[ocnt++] = c;
	if (ocnt >= sizeof obuf)
		oflush();
}

outstr(s)
register char	*s;
{
	while (*s)
		outc(*s++);
}

oflush()
{
	if (ocnt > 0)
		write(1, obuf, ocnt);
	ocnt = 0;
}
