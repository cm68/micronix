/*
 * concat - concatenate files onto the standard output
 *
 * cmd/concat/concat.c
 *
 * A reconstruction of /usr/bin/concat off the Micronix 1.6 distribution -
 * inode 59 of 1012-8_dist_2.IMD - from its disassembly.  There is no
 * source for it.  README says how it was read.  It is the program of that
 * name in Kernighan and Plauger's Software Tools.
 *
 * concat file ...
 *
 * Each file is copied to the standard output in turn, in blocks of 512
 * bytes.  With no files nothing is written, and the standard input is not
 * read.  A file that will not open is the end of the run - "name: can't
 * open" on the standard error - after what came before it has been
 * written.
 *
 * THE ORIGINAL COPIED ONE LINE OF EACH FILE.  Its loop ran until the
 * library's block write came back with 0, and that write does come back
 * with 0 - it flushes the standard output, which it does when a block
 * ends in a newline, and gives the flush's value, not the count - so
 * after the first line (or the first 512 characters of a line with no
 * end to it) the loop ended and the next file was begun.  This copies
 * the whole of each file, as the manual says it does; README has the
 * evidence.
 *
 * The original called exit with 0 for the file that would not open and
 * with whatever was left in BC when it fell off the end of the list, so
 * its good status was a number that meant nothing.  It is 0 and 1 here.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

#define	BLOCK	512

char	buf[BLOCK];

cantopen(name)
	char *name;
{
	write(2, name, strlen(name));
	write(2, ": can't open\n", 13);
	exit(1);
}

main(argc, argv)
	int argc;
	char *argv[];
{
	register int i, n;
	int fd;

	for (i = 1; i < argc; i++) {
		fd = open(argv[i], 0);
		if (fd < 0)
			cantopen(argv[i]);
		while ((n = read(fd, buf, BLOCK)) > 0)
			fwrite(buf, 1, n, stdout);
		close(fd);
	}
	exit(0);
}
