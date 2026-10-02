/*
 * overstrike - turn backspaces into lines of their own
 *
 * cmd/overstrike/overstrike.c
 *
 * A reconstruction of /usr/bin/overstrike off the Micronix 1.6
 * distribution - inode 29 of 1013-8_dist_3.IMD - from its disassembly.
 * There is no source for it.  README says how it was read.  It is the
 * program of that name in Kernighan and Plauger's Software Tools.
 *
 * A filter, for a printer that cannot back up.  Two columns are kept,
 * the one the output has reached and the one the input has: a backspace
 * moves the second back by one, to column 1 at the least, and nothing is
 * written for it.  The next character that is not a backspace, if the
 * input is behind the output, is preceded by a carriage return and by
 * the blanks that bring the line to the input's column - so the text
 * after a backspace is written over the text before it on a second pass
 * along the line.  A newline starts both columns again at 1.  Nothing
 * else is looked at, so a tab is one column.
 *
 * The blanks are written even when the file ends: a line that is backed
 * up at its end is followed by the carriage return and the blanks, and
 * nothing after them.
 *
 * The original exited with the -1 that getc gave at the end of the file
 * in BC, which is status 255.  This exits 0.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

main()
{
	register int c;
	int ocol, icol;

	ocol = 1;
	for (;;) {
		icol = ocol;
		while ((c = getchar()) == '\b')
			icol = icol - 1 < 1 ? 1 : icol - 1;
		if (icol < ocol) {
			putchar('\r');
			for (ocol = 1; ocol < icol; ocol++)
				putchar(' ');
		}
		if (c == EOF)
			break;
		putchar(c);
		if (c == '\n')
			ocol = 1;
		else
			ocol++;
	}
	exit(0);
}
