/*
 * kwic - key word in context index
 *
 * cmd/kwic/kwic.c
 *
 * A reconstruction of /usr/bin/kwic off the Micronix 1.6 distribution -
 * inode 54 of 1012-8_dist_2.IMD - from its disassembly.  There is no
 * source for it.  README says how it was read.  It is the program of
 * that name in Kernighan and Plauger's Software Tools, and a pipeline
 * with sort and unrot makes the index:
 *
 *	kwic <text | sort | unrot >index
 *
 * A filter.  For each line of the input it writes one line for each
 * word in it - a word being a run of letters and digits - and the line
 * is the line rotated so that the word comes first: the word and what
 * follows it, a dollar sign, and what came before.  So
 *
 *	the quick fox
 *
 * is written
 *
 *	the quick fox$
 *	quick fox$the<blank>
 *	fox$the quick<blank>
 *
 * with the blank that was before the word, which is not trimmed.
 *
 * Tabs in a line are blanks.  Lines are read in pieces of at most 512
 * characters, a newline included, and each piece is a line of its own;
 * so a longer line is taken as two or more.
 *
 * The original exited with whatever was in BC when the input ended,
 * which was 0, so it did nothing different.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

#define	LINESZ	512

char	line[LINESZ + 1];

alnum(c)
	register int c;
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
	    (c >= '0' && c <= '9');
}

/*
 * A character out.  The original passed it to putc as a char widened to
 * an int, and putc of a negative number is the request to flush, so a
 * character with the high bit set is never written: it is lost from the
 * line.  Text is ASCII, and the original's index of anything else is
 * what it is.
 */
out(c)
	register int c;
{
	if (!(c & 0200))
		putchar(c);
}

/*
 * One rotation: the word to the end of the line, the dollar sign, and
 * the line up to the word.  (025f)
 */
rotate(s, word)
	register char *s, *word;
{
	register char *p;

	for (p = word; *p && *p != '\n'; p++)
		out(*p);
	out('$');
	for (p = s; p < word; p++)
		out(*p);
	out('\n');
}

/*
 * A line, a rotation for each of its words.  (0168)
 */
kwic(s)
	char *s;
{
	register char *p;

	for (p = s; *p; p++)
		if (*p == '\t')
			*p = ' ';
	p = s;
	while (*p && *p != '\n') {
		if (alnum(*p)) {
			rotate(s, p);
			while (alnum(*p))
				p++;
		} else
			p++;
	}
}

/*
 * Up to LINESZ characters, as far as a newline.
 */
getl(s)
	register char *s;
{
	register int c, n;

	n = 0;
	while (n < LINESZ && (c = getchar()) != EOF) {
		s[n++] = c;
		if (c == '\n')
			break;
	}
	return n;
}

main()
{
	register int n;

	while ((n = getl(line)) > 0) {
		line[n] = '\0';
		kwic(line);
	}
	exit(0);
}
