/*
 * entab - replace blanks by tabs
 *
 * cmd/entab/entab.c
 *
 * A reconstruction of /usr/bin/entab off the Micronix 1.6 distribution -
 * inode 56 of 1012-8_dist_2.IMD - from its disassembly.  There is no
 * source for it.  README says how it was read.
 *
 * A filter.  Standard input goes to standard output with runs of blanks
 * replaced by tabs where a tab reaches the same column; the stops are at
 * 9, 17, 25 and so on.  Two columns are kept, the one the input has
 * reached and the one the output has, and nothing is written for a blank
 * until a visible character needs it, so:
 *
 *	blanks and tabs at the end of a line are dropped, and so are
 *	those at the end of the file;
 *	a backspace moves the input column back, and the output is
 *	brought back to it with backspaces if it is already past it;
 *	a gap of one column is a blank and never a tab, even when the
 *	tab would reach it;
 *	a newline or carriage return writes itself and starts both
 *	columns again;
 *	any other character that is not printable - a form feed, a
 *	byte over 0176 - is written as it stands and takes no column.
 *
 * The original called exit with whatever getc left in BC, which at end
 * of file is 255.  This exits 0.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

int	icol;
int	ocol;

/*
 * Bring the output to the input's column.  (031b)
 */
catchup()
{
	int nt;

	while (icol < ocol) {
		putchar('\b');
		ocol--;
	}
	if (ocol + 1 == icol) {
		putchar(' ');
		ocol++;
	}
	for (;;) {
		nt = (ocol & ~7) + 8;
		if (icol < nt)
			break;
		putchar('\t');
		ocol = nt;
	}
	while (ocol < icol) {
		putchar(' ');
		ocol++;
	}
}

main()
{
	register int c;

	while ((c = getchar()) != EOF) {
		if (c == ' ') {
			icol++;
		} else if (c == '\t') {
			icol = (icol & ~7) + 8;
		} else if (c == '\b') {
			if (icol)
				icol--;
		} else if (c == '\n' || c == '\r') {
			putchar(c);
			icol = ocol = 0;
		} else if (c < '!' || c > '~') {
			catchup();
			putchar(c);
		} else {
			catchup();
			putchar(c);
			icol++;
			ocol++;
		}
	}
	exit(0);
}
