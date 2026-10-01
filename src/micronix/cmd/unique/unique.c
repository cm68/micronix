/*
 * unique [-n]
 *
 * v7 uniq (usr/src/cmd/uniq.c), ported to micronix.
 *
 * cmd/unique/unique.c
 *
 * The name is micronix's: /bin says unique and the 1982 page is unique.1,
 * while the v7 source is uniq.c and the command there is uniq.
 *
 * The one flag is micronix's too.  v7 counts each run with -c, in a field
 * four wide; this counts with -n, which is what the page documents, and
 * prints the count as a number and a tab.  The count is counted the same
 * way and the lines are compared the same way - the loop and the two
 * routines under it are v7's, minus what went.
 *
 * What the port took out, and why:
 *
 *	-u -d		v7's "the uniques only" and "the repeats only".
 *	+n		v7's skip of n characters before comparing.
 *	-n <number>	v7's other use of the flag, skipping n fields.
 *	skip()		the one routine the last two shared.
 *	the two files	v7 takes an input file and an output file and
 *			reopens them onto the standard streams.
 *
 * The 1982 binary has none of those either, and each was checked against
 * it rather than assumed.  It reads the standard input and writes the
 * standard output; it takes no files; and its option scan looks at the
 * first argument and no further, so that -u, -d, +2, a file name and
 * anything else that is not -n all fall through to the plain copy.
 * "unique /etc/rc" does not open /etc/rc - it reads the standard input,
 * which is what the 1982 binary does with it.
 *
 * The thousand byte line buffers are v7's, and the bound on them is not.
 * v7 wrote into them with nothing watching, and so does the 1982 binary:
 * feed either a line of two thousand characters and the line runs out of
 * its buffer and the output is wrong from there on - wrong differently in
 * the two, because what the overrun writes over is whatever the compiler
 * put next.  Here that is the rest of the program's own data: the two
 * buffers are not at the end of it, so the damage stops at the segment
 * boundary, and what comes out is garbage rather than a fault.  A bound
 * is the smallest thing that stops it, and it costs the case it catches:
 * a line of a thousand characters or more is refused with a message
 * instead of being mangled, which is what the 1982 tail says about the
 * same line.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>

/*
 * puts is in libu.a - lib/libu/puts.c, with fputs beside it - and in no
 * header.  Declared here so the call below is a call and not an implicit
 * int.
 */
int	puts();

/*
 * The line buffer, and the longest line that fits in it.  v7's number; the
 * name and the check that uses it are this port's.
 */
#define	LBUF	1000

int	linec;			/* how many lines the run being printed ran */
char	mode;			/* 'n' when the counts are wanted, 0 when not */

main(argc, argv)
int argc;
char **argv;
{
	static char b1[LBUF], b2[LBUF];

	/*
	 * The first argument and only the first.  argv[1][1] is tested
	 * rather than the whole word so that -n is -n whatever follows
	 * it, which is how the flag reads and how the 1982 binary takes
	 * it.
	 */
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'n')
		mode = 'n';

	if (gline(b1))
		exit(0);
	for (;;) {
		linec++;
		if (gline(b2)) {
			pline(b1);
			exit(0);
		}
		if (!equal(b1, b2)) {
			pline(b1);
			linec = 0;
			do {
				linec++;
				if (gline(b1)) {
					pline(b2);
					exit(0);
				}
			} while (equal(b1, b2));
			pline(b2);
			linec = 0;
		}
	}
}

/*
 * One line, without its newline, into buf.  Returns 1 at end of file,
 * which is the only way a line can be missing, so a file whose last line
 * has no newline loses that line - v7's gline, and v7's behavior.
 *
 * The length is counted, which v7 did not do - see the head of the file.
 */
gline(buf)
register char buf[];
{
	register c, n;

	for (n = 0; (c = getchar()) != '\n'; n++) {
		if (c == EOF)
			return (1);
		if (n >= LBUF - 1)
			fatal("line too long");
		*buf++ = c;
	}
	*buf = 0;
	return (0);
}

fatal(s)
char *s;
{

	fprintf(stderr, "unique: %s\n", s);
	exit(1);
}

pline(buf)
register char buf[];
{

	if (mode == 'n')
		printf("%d\t", linec);
	puts(buf);
}

equal(b1, b2)
register char b1[], b2[];
{
	register char c;

	while ((c = *b1++) != 0)
		if (c != *b2++)
			return (0);
	if (*b2 != 0)
		return (0);
	return (1);
}
