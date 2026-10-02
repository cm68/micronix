/*
 * uuencode - convert a file to ASCII for transmission
 *
 * cmd/uuencode/uuencode.c
 *
 * Reconstructed from /bin/uuencode off the Micronix 1.67 patch
 * (disks/Late/mw1.67_patch/bin/uuencode), a Whitesmith's binary with no
 * source.  It is the standard uuencode, the one the man page promises is
 * "compatible with standard unix uuencode": three bytes become four
 * six-bit characters each offset by 0x20, forty-five input bytes make
 * one output line, and the line is prefixed by its own byte count in
 * the same offset.
 *
 * The body is wrapped in "begin %o %s" and "end".  The %o mode is the
 * input file's st_mode masked to 0777 - read out of the disassembly as
 * stat() then (statbuf+4) & 0x1ff, so it is the file's real mode, not a
 * fixed 0644.
 *
 * One deliberate divergence: the binary encodes the padding of a final
 * short group out of whatever the stack held past the read, so a file
 * whose length is not a multiple of three is not reproducible byte for
 * byte.  This zero-pads that group instead, which uudecode reads back
 * identically - the length byte says how many of the bytes are real.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
#include <stdio.h>
#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>

#define	LEN	45

char	buf[LEN];

main(argc, argv)
char **argv;
{
	register FILE *in;
	register int n, i;
	register char *name;
	struct stat st;
	register int c1, c2, c3;
	int mode;

	if (argc < 2 || argc > 3) {
		printf("Usage: uuencode [infile] remotefile\n");
		exit(2);
	}
	in = fopen(argv[1], "r");
	if (in == NULL) {
		perror(argv[1]);
		exit(1);
	}
	name = argc > 2 ? argv[2] : argv[1];
	mode = stat(argv[1], &st) == 0 ? st.st_mode & 0777 : 0644;
	printf("begin %o %s\n", mode, name);
	while ((n = fread(buf, 1, LEN, in)) > 0) {
		putchar(n + ' ');
		for (i = 0; i < n; i += 3) {
			c1 = buf[i] & 0377;
			c2 = i + 1 < n ? buf[i + 1] & 0377 : 0;
			c3 = i + 2 < n ? buf[i + 2] & 0377 : 0;
			putchar((c1 >> 2) + ' ');
			putchar((((c1 << 4) & 060) | (c2 >> 4)) + ' ');
			putchar((((c2 << 2) & 074) | (c3 >> 6)) + ' ');
			putchar((c3 & 077) + ' ');
		}
		putchar('\n');
	}
	putchar(' ');
	putchar('\n');
	printf("end\n");
	exit(0);
}
