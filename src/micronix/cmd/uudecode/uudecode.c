/*
 * uudecode - convert an ASCII uuencoded file back to binary
 *
 * cmd/uudecode/uudecode.c
 *
 * Reconstructed from /bin/uudecode off the Micronix 1.67 patch
 * (disks/Late/mw1.67_patch/bin/uudecode), a Whitesmith's binary with no
 * source.  It is the standard uudecode, the reverse of uuencode(1): it
 * reads the "begin %o %s" line, creates the named file with the %o mode,
 * and reverses the four-six-bit-characters-to-three-bytes transform line
 * by line until the "end" line.
 *
 * The errors it prints - "Short file" and "No end line" - are the two
 * the binary's own strings carry, so this reproduces them.  uudecode
 * writes the file and chmod's it to the begin line's mode.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
#include <stdio.h>
#include <string.h>

#define	LEN	45

char	line[4 * LEN + 8];

main(argc, argv)
char **argv;
{
	register FILE *in, *out;
	register char *p;
	register int n;
	int i, c1, c2, c3, c4;
	char name[64];
	int mode;

	if (argc > 2) {
		printf("Usage: uudecode [infile]\n");
		exit(1);
	}
	if (argc == 2) {
		in = fopen(argv[1], "r");
		if (in == NULL) {
			perror(argv[1]);
			exit(1);
		}
	} else
		in = stdin;

	/* find the begin line, skipping anything before it */
	for (;;) {
		if (fgets(line, sizeof line, in) == NULL) {
			fprintf(stderr, "No end line\n");
			exit(1);
		}
		if (strncmp(line, "begin ", 6) == 0)
			break;
	}
	sscanf(line, "begin %o %s", &mode, name);
	out = fopen(name, "w");
	if (out == NULL) {
		perror(name);
		exit(1);
	}
	chmod(name, mode);

	for (;;) {
		if (fgets(line, sizeof line, in) == NULL) {
			fprintf(stderr, "No end line\n");
			exit(1);
		}
		if (strncmp(line, "end", 3) == 0)
			break;
		n = line[0] - ' ';
		p = line + 1;
		for (i = 0; i < n; i += 3) {
			c1 = *p++ - ' ';
			c2 = *p++ - ' ';
			c3 = *p++ - ' ';
			c4 = *p++ - ' ';
			putc((c1 << 2) | (c2 >> 4), out);
			if (i + 1 < n)
				putc((c2 << 4) | (c3 >> 2), out);
			if (i + 2 < n)
				putc((c3 << 6) | c4, out);
		}
	}
	exit(0);
}
