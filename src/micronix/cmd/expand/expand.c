/*
 * expand - undo compress
 *
 * cmd/expand/expand.c
 *
 * A reconstruction of /usr/bin/expand off the Micronix 1.6 distribution -
 * inode 55 of 1012-8_dist_2.IMD - from its disassembly.  There is no
 * source for it.  README says how it was read.
 *
 * expand [ input [ output ] ]
 *
 * It reads the blocks compress writes - see compress.c for them - and
 * writes what they stood for: a count from 0 to 254 followed by that
 * many bytes is copied, and the byte 0377 followed by a byte and a count
 * is that byte, that many times.  A count of 255 is not a literal count
 * and never reaches the second branch, so 0377 always means a run.
 *
 * It is no more careful than the original, which the manual says was
 * not guaranteed to do anything sensible with input that did not come
 * from compress: a block that ends short at the end of the file is
 * written as far as it goes, and a literal count of 0 is a block of
 * nothing.
 *
 * The files are binary and read and written by the byte out of a buffer,
 * as in compress.  The output is created mode 0777.  Mistakes are the
 * usage line and "name: can't open." on the standard error, with exit 1
 * where the original had exit(0); a clean run exits 0 where the original
 * exited with whatever was in BC.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

#define	BUFSZ	512
#define	RUN	0377

int	fi;
int	fo;
char	ibuf[BUFSZ];
char	obuf[BUFSZ];
int	icnt;
int	ipos;
int	ocnt;

fail(a, b)
	char *a, *b;
{
	if (a)
		write(2, a, strlen(a));
	write(2, b, strlen(b));
	write(2, "\n", 1);
	exit(1);
}

usage()
{
	fail((char *)0, "usage: expand [ input [ output ] ].");
}

cantopen(name)
	char *name;
{
	fail(name, ": can't open.");
}

getb()
{
	if (ipos >= icnt) {
		icnt = read(fi, ibuf, BUFSZ);
		ipos = 0;
		if (icnt <= 0) {
			icnt = 0;
			return EOF;
		}
	}
	return ibuf[ipos++] & 0377;
}

flush()
{
	if (ocnt) {
		write(fo, obuf, ocnt);
		ocnt = 0;
	}
}

putb(c)
	int c;
{
	if (ocnt == BUFSZ)
		flush();
	obuf[ocnt++] = c;
}

main(argc, argv)
	int argc;
	char *argv[];
{
	register int n, b;

	fi = 0;
	fo = 1;
	if (argc > 3)
		usage();
	if (argc >= 2) {
		fi = open(argv[1], 0);
		if (fi < 0)
			cantopen(argv[1]);
	}
	if (argc >= 3) {
		fo = creat(argv[2], 0777);
		if (fo < 0)
			cantopen(argv[2]);
	}

	for (;;) {
		n = getb();
		if (n == EOF)
			break;
		if (n == RUN) {
			b = getb();
			if (b == EOF)
				break;
			n = getb();
			if (n == EOF)
				break;
			while (n-- > 0)
				putb(b);
		} else {
			while (n-- > 0) {
				b = getb();
				if (b == EOF)
					goto done;
				putb(b);
			}
		}
	}
done:
	flush();
	exit(0);
}
