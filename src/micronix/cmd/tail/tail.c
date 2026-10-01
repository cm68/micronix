/*
 * tail [-|+]n[l|b|c|r] [file]
 *
 * v7 tail (usr/src/cmd/tail.c), ported to micronix.
 *
 * cmd/tail/tail.c
 *
 * The program is v7's, whole.  tail is on the system and this is the
 * source for it, and nothing above or below the loop below is a
 * redesign: the argument grammar, the two directions, the search
 * backwards through the buffer and the odd cases at the ends of the file
 * are all as v7 wrote them.  What changed is the spelling of the
 * machine, and it is listed at the bottom of this comment.
 *
 * The interface the man page fixes is the 1982 page's - "tail [-N]
 * [file]", the last N lines of the file, ten by default - and that is
 * the -n form here.  The page does not mention the rest of v7's
 * grammar, and the port keeps it: a "+n" counts from the beginning
 * instead of the end, and a letter after the number says what is being
 * counted - l for lines, b for 512 byte blocks, c for characters, r for
 * the lines in reverse.  The page fixes that much and no more.
 *
 * THE 1982 BINARY IS A DIFFERENT PROGRAM, and this is not it.  Its
 * messages are its own - "tail: line too long", "tail: Out of memory",
 * "%s: can't open" - and it reads the whole file in, which is what the
 * page's BUGS section describes: "Tail reads the entire file in order to
 * obtain the last few lines."  More than that, it does not take -N at
 * all: run it as "tail -3 file" and it exits 1 with nothing written and
 * nothing said, because the digit reaches a case the switch does not
 * have.  Given the file name alone, or nothing and a standard input, it
 * prints the last ten lines.  This port follows the page, which
 * documents -N, and does what the page says with it.
 *
 * What the port changed, all of it machine things:
 *
 *	errno		v7 declares its own.  The tree's <errno.h> carries
 *			the numbers and not the variable, so this declares
 *			it the way cmd/ln does.
 *	st_size		the micronix inode keeps a file's size as a high
 *			byte over a low word and there is no st_size to
 *			read.  T_SIZE below is find's and tar's arithmetic.
 *	the includes	<sys/types.h> is <types.h> here, and <sys/fs.h>
 *			has to come before <sys/stat.h>, because the
 *			stat structure names the inode that lives there.
 *
 * The "usage" line is v7's own, with its "+_n" put right.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/fs.h>
#include <sys/stat.h>

#define	LBIN	4097

/*
 * A file's size, out of the two fields the micronix inode keeps it in:
 * d_size0 is the high byte and d_size1 the low word.  That is
 * twenty-four bits, so a long holds any of it.
 */
#define	T_SIZE(sp)	(((long)(sp)->st_size0 << 16) + (sp)->st_size1)

/*
 * <string.h> declares this, and so does <sys.h>; the declaration is here
 * as well because this is the file that reads it and the file should say
 * where it comes from.
 */
extern int	errno;

struct	stat	statb;
char	bin[LBIN];

main(argc, argv)
int argc;
char **argv;
{
	long n, di;
	register int i, j, k;
	char *p;
	int partial, piped, bylines, bkwds, fromend, lastnl;
	char *arg;

	lseek(0, (long)0, 1);
	piped = errno == ESPIPE;
	arg = argv[1];
	if (argc <= 1 || *arg != '-' && *arg != '+') {
		arg = "-10l";
		argc++;
		argv--;
	}
	fromend = *arg == '-';
	arg++;
	n = 0;
	while (digit(*arg))
		n = n*10 + *arg++ - '0';
	if (!fromend && n > 0)
		n--;
	if (argc > 2) {
		close(0);
		if (open(argv[2], 0) != 0) {
			write(2, "tail: can't open ", 17);
			write(2, argv[2], strlen(argv[2]));
			write(2, "\n", 1);
			exit(1);
		}
	}
	bylines = 0;
	bkwds = 0;
	switch (*arg) {
	case 'b':
		n <<= 9;
		break;
	case 'c':
		break;
	case 'r':
		if (n == 0)
			n = LBIN;
		bkwds = 1;
		fromend = 1;
		bylines = 1;
		break;
	case '\0':
	case 'l':
		bylines = 1;
		break;
	default:
		goto errcom;
	}
	if (fromend)
		goto keep;

			/*seek from beginning */

	if (bylines) {
		j = 0;
		while (n-- > 0) {
			do {
				if (j-- <= 0) {
					p = bin;
					j = read(0, p, 512);
					if (j-- <= 0)
						exit(0);
				}
			} while (*p++ != '\n');
		}
		write(1, p, j);
	} else if (n > 0) {
		if (!piped)
			fstat(0, &statb);
		if (piped || (statb.st_mode & S_IFMT) == S_IFCHR)
			while (n > 0) {
				i = n > 512 ? 512 : n;
				i = read(0, bin, i);
				if (i <= 0)
					exit(0);
				n -= i;
			}
		else
			lseek(0, n, 0);
	}
copy:
	while ((i = read(0, bin, 512)) > 0)
		write(1, bin, i);
	exit(0);

			/*seek from end*/

keep:
	if (n <= 0)
		exit(0);
	if (!piped) {
		fstat(0, &statb);
		di = !bylines ? n : LBIN-1;
		if (T_SIZE(&statb) > di)
			lseek(0, -di, 2);
		if (!bylines)
			goto copy;
	}
	partial = 1;
	for (;;) {
		i = 0;
		do {
			j = read(0, &bin[i], LBIN-i);
			if (j <= 0)
				goto brka;
			i += j;
		} while (i < LBIN);
		partial = 0;
	}
brka:
	if (!bylines) {
		k =
		    n <= i ? i-n :
		    partial ? 0:
		    n >= LBIN ? i+1:
		    i-n+LBIN;
		k--;
	} else {
		if (bkwds && bin[i == 0 ? LBIN-1 : i-1] != '\n') {	/* force trailing newline */
			bin[i] = '\n';
			if (++i >= LBIN) {
				i = 0;
				partial = 0;
			}
		}
		k = i;
		j = 0;
		do {
			lastnl = k;
			do {
				if (--k < 0) {
					if (partial) {
						if (bkwds)
							write(1, bin, lastnl+1);
						goto brkb;
					}
					k = LBIN - 1;
				}
			} while (bin[k] != '\n' && k != i);
			if (bkwds && j > 0) {
				if (k < lastnl)
					write(1, &bin[k+1], lastnl-k);
				else {
					write(1, &bin[k+1], LBIN-k-1);
					write(1, bin, lastnl+1);
				}
			}
		} while (j++ < n && k != i);
brkb:
		if (bkwds)
			exit(0);
		if (k == i)
			do {
				if (++k >= LBIN)
					k = 0;
			} while (bin[k] != '\n' && k != i);
	}
	if (k < i)
		write(1, &bin[k+1], i-k-1);
	else {
		write(1, &bin[k+1], LBIN-k-1);
		write(1, bin, i);
	}
	exit(0);
errcom:
	write(2, "usage: tail [-|+]n[lbcr] [file]\n", 32);
	exit(1);
}

digit(c)
{
	return (c >= '0' && c <= '9');
}
