/*
 * copy - copy standard input to standard output
 *
 * cmd/copy/copy.c
 *
 * A reconstruction of /usr/bin/copy off the Micronix 1.6 distribution -
 * inode 58 of 1012-8_dist_2.IMD - from its disassembly.  There is no
 * source for it.  README says how it was read.  It is the program of
 * that name in Kernighan and Plauger's Software Tools.
 *
 * It copies the standard input to the standard output, a character at a
 * time, to the end of the file.  Every byte is copied as it is.
 *
 * The original left the -1 that getc gave at the end of the file in BC
 * and exited with it, so every run of it ended with status 255.  This
 * exits 0.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

main()
{
	register int c;

	while ((c = getchar()) != EOF)
		putchar(c);
	exit(0);
}
