/*
 * detab - convert tabs to spaces
 *
 * cmd/detab/detab.c
 *
 * A reconstruction of /usr/bin/detab off the Micronix 1.6 distribution -
 * inode 57 of 1012-8_dist_2.IMD - from its disassembly.  There is no
 * source for it.  README says how it was read.
 *
 * A filter: standard input to standard output with every tab replaced by
 * the spaces that reach the next tab stop, and the stops are at columns
 * 9, 17, 25 and so on.  The column count starts at 1 and goes back to it
 * after a newline; a carriage return or a backspace is a column like any
 * other, as it is in the original.
 *
 * The original called exit with whatever getc left in BC, which at end
 * of file is -1, so it always exited 255.  This exits 0.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

main()
{
	register int c;
	int col;

	col = 1;
	while ((c = getchar()) != EOF) {
		if (c == '\t') {
			do {
				putchar(' ');
				col++;
			} while (col % 8 != 1);
		} else if (c == '\n') {
			putchar(c);
			col = 1;
		} else {
			putchar(c);
			col++;
		}
	}
	exit(0);
}
