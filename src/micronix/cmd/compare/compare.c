/*
 * compare - compare two files line by line
 *
 * cmd/compare/compare.c
 *
 * A reconstruction of /usr/bin/compare off the Micronix 1.6 distribution -
 * inode 61 of 1012-8_dist_2.IMD - from its disassembly.  There is no
 * source for it.  README says how it was read.  It is the program of that
 * name in Kernighan and Plauger's Software Tools.
 *
 * compare file1 file2
 *
 * The files are read together a line at a time, and each line of one is
 * set against the line of the other.  When two lines are not the same
 * what is written is the number of the line, a newline, the line from
 * the first file and the line from the second, each with the newline it
 * was read with - so a line that has none, which is only the last of a
 * file, runs into the next thing written.  Nothing is written for files
 * that are the same.
 *
 * As the manual says, it is no cleverer than that: a line added to one of
 * the files puts every line after it out of step.
 *
 * It stops when either file ends, and does not say which, or that the
 * other goes on.  The original has the words for it - "eof on file 1."
 * and "eof on file 2." - and two tests on variables that nothing ever
 * sets, so they are made on whatever the stack had in it, and on a
 * stack that is clear are not made at all.  They are not made here.
 * What ends the run is the end of either file *met*, at the end of a line
 * or in the middle of one, so a last line with no newline is not
 * compared, the same as in include.
 *
 * Lines are read 511 characters at a time, newline included, and a longer
 * line is two.  The usage line and "name: can't open." are written to the
 * standard error, and both were an exit with 0, the good end being an
 * exit with 1; they are 1 and 0 here.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

#define	LINESZ	512

char	line1[LINESZ];
char	line2[LINESZ];

die(a, b)
	char *a, *b;
{
	if (a)
		write(2, a, strlen(a));
	write(2, b, strlen(b));
	write(2, "\n", 1);
	exit(1);
}

cantopen(name)
	char *name;
{
	die(name, ": can't open.");
}

/*
 * A line of at most n - 1 characters, newline included, and its end -
 * the NUL after it.  Whether the file ended is the answer, and it is so
 * if the end was met at all.  (085e)
 */
getl(fp, s, n)
	FILE *fp;
	register char *s;
	register int n;
{
	register int c;

	while (--n > 0) {
		c = getc(fp);
		if (c == EOF) {
			*s = '\0';
			return 1;
		}
		*s++ = c;
		if (c == '\n')
			break;
	}
	*s = '\0';
	return 0;
}

/*
 * The line number, and the two lines.  (0359)
 */
differ(n, s1, s2)
	long n;
	char *s1, *s2;
{
	char buf[16];

	sprintf(buf, "%ld\n", n);
	write(1, buf, strlen(buf));
	write(1, s1, strlen(s1));
	write(1, s2, strlen(s2));
}

main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *f1, *f2;
	long n;
	int e1, e2;

	if (argc < 3)
		die((char *)0, "usage: compare file1 file2.");
	f1 = fopen(argv[1], "r");
	if (f1 == NULL)
		cantopen(argv[1]);
	f2 = fopen(argv[2], "r");
	if (f2 == NULL)
		cantopen(argv[2]);
	n = 0;
	for (;;) {
		e1 = getl(f1, line1, LINESZ);
		e2 = getl(f2, line2, LINESZ);
		if (e1 || e2)
			break;
		n++;
		if (strcmp(line1, line2) != 0)
			differ(n, line1, line2);
	}
	exit(0);
}
