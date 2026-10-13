/*
 * split - split a file into pieces
 *
 * cmd/split/split.c
 *
 * Reads the named file (or the standard input) and writes it back out
 * in n-line pieces.  The input is only read.  The piece files are the
 * prototype name with a two letter suffix - "xaa", "xab", and so on -
 * and the prototype defaults to "x" and the count to 1000 lines.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <stdlib.h>

char	outname[64];
int	eof;

static void
nextname(a)
	char *a;
{
	register char *p;

	for (p = a; *p; p++)
		;
	for (p--; *p == 'z'; p--)
		;
	(*p)++;
	for (p++; *p; p++)
		*p = 'a';
}

main(argc, argv)
	int argc;
	char **argv;
{
	int i, nlines, byblock;
	char *infile, *proto;
	FILE *in, *out;
	register int c;
	int n;
	long count;

	nlines = 0;
	byblock = 0;
	infile = (char *)0;
	proto = "x";
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 'b')
			byblock = 1;
		else if (argv[i][0] == '-' && argv[i][1] >= '0' &&
			 argv[i][1] <= '9')
			nlines = atoi(argv[i] + 1);
		else if (infile == 0)
			infile = argv[i];
		else
			proto = argv[i];
	}
	if (nlines <= 0)
		nlines = 1000;
	if (infile == 0 || strcmp(infile, "-") == 0) {
		in = stdin;
		infile = "-";
	} else if ((in = fopen(infile, "r")) == NULL) {
		fprintf(stderr, "split: can't open %s\n", infile);
		exit(1);
	}
	strcpy(outname, proto);
	strcat(outname, "aa");
	eof = 0;
	while (!eof) {
		out = fopen(outname, "w");
		if (out == NULL) {
			fprintf(stderr, "split: can't create %s\n", outname);
			exit(1);
		}
		count = 0;
		if (byblock) {
			for (n = 0; n < nlines && !eof; n++) {
				for (i = 0; i < BUFSIZ; i++) {
					if ((c = getc(in)) == EOF) {
						eof = 1;
						break;
					}
					putc(c, out);
				}
			}
		} else {
			while (count < nlines && !eof) {
				if ((c = getc(in)) == EOF) {
					eof = 1;
					break;
				}
				putc(c, out);
				if (c == '\n')
					count++;
			}
		}
		fclose(out);
		if (!eof)
			nextname(outname);
	}
	exit(0);
}
