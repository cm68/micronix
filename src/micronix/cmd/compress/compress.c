/*
 * compress - squeeze runs of identical characters
 *
 * cmd/compress/compress.c
 *
 * A reconstruction of /usr/bin/compress off the Micronix 1.6
 * distribution - inode 60 of 1012-8_dist_2.IMD - from its disassembly.
 * There is no source for it.  README says how it was read.
 *
 * compress [ input [ output ] ]
 *
 * The output is a string of blocks, and expand is the program that reads
 * them back.  There are two kinds:
 *
 *	n  b1 b2 ... bn		a literal block: a count from 1 to 254 and
 *				that many bytes, copied
 *	377  b  n		a run block: the byte 0377, the byte, and
 *				a count from 5 to 254 of how many of it
 *
 * A run of five or more of one byte is a run block, and a run of more
 * than 254 is a series of them.  Anything shorter stays literal, and
 * literal bytes are held until there are 254 of them or a run block has
 * to go out between them.  Because a literal count is never 255, the
 * byte 0377 can only be the start of a run block.
 *
 * The files are opened as binary, and what is read is what is written;
 * the original did this with its own stream and so does this, a byte at
 * a time out of a buffer, so that nothing here depends on how stdio
 * treats a carriage return.  The output is created mode 0777, as the
 * original's create did.
 *
 * Mistakes are reported as the original reported them: the usage line,
 * or the name of the file followed by ": can't open." and a newline, on
 * the standard error.  The original then called exit(0).  This exits 1.
 * At the end of a run it called exit with whatever was in BC, which is
 * not a status; this exits 0.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

#define	BUFSZ	512
#define	MAXLIT	254
#define	MAXRUN	254
#define	MINRUN	5
#define	RUN	0377

int	fi;
int	fo;
char	ibuf[BUFSZ];
char	obuf[BUFSZ];
int	icnt;
int	ipos;
int	ocnt;
char	lit[MAXLIT];
int	nlit;

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
	fail((char *)0, "usage: compress [ input [ output ] ].");
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

/*
 * Send the literal bytes held.  (034a)
 */
putlit()
{
	register int i;

	if (nlit <= 0)
		return;
	putb(nlit);
	for (i = 0; i < nlit; i++)
		putb(lit[i]);
	nlit = 0;
}

main(argc, argv)
	int argc;
	char *argv[];
{
	register int c, d;
	int run;

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

	c = getb();
	while (c != EOF) {
		run = 1;
		while ((d = getb()) == c && run < MAXRUN)
			run++;
		if (run >= MINRUN) {
			putlit();
			putb(RUN);
			putb(c);
			putb(run);
		} else {
			while (run > 0) {
				lit[nlit++] = c;
				if (nlit >= MAXLIT)
					putlit();
				run--;
			}
		}
		c = d;
	}
	putlit();
	flush();
	exit(0);
}
