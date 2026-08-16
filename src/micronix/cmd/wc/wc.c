/*
 * wc - count lines, words and characters
 *
 * cmd/wc/wc.c
 *
 * Written for this tree - wc is not in the 2.11 snapshot, which
 * carries bin and not usr.bin, and the system never had one.  The
 * shape is the seventh edition's: count lines, words and characters
 * for each named file or the standard input, print the counts that
 * were asked for - all three when nothing was - and a total line
 * when there was more than one file.
 *
 * A word is a maximal run of characters that are not space, tab or
 * newline, which is the only definition wc has ever needed.  The
 * counts are longs: a character count is a file size, and file
 * sizes outgrew sixteen bits before this machine was built.  The
 * three flags are char, by the range audit's law.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>

char	lflag;
char	wflag;
char	cflag;
char	anyflag;			/* a flag was given at all */

long	tlines, twords, tchars;

main(argc, argv)
int argc;
char *argv[];
{
	register int i;
	int nfiles;
	FILE *f;
	char *p;

	argc--;
	argv++;
	if (argc > 0 && argv[0][0] == '-' && argv[0][1]) {
		for (p = &argv[0][1]; *p; p++) {
			switch (*p) {
			case 'l':
				lflag = 1;
				break;
			case 'w':
				wflag = 1;
				break;
			case 'c':
				cflag = 1;
				break;
			default:
				fprintf(stderr, "usage: wc [-lwc] [file ...]\n");
				exit(1);
			}
		}
		anyflag = 1;
		argc--;
		argv++;
	}
	if (!anyflag)
		lflag = wflag = cflag = 1;

	if (argc == 0) {
		count(stdin, (char *)0);
		exit(0);
	}
	nfiles = argc;
	for (i = 0; i < argc; i++) {
		if ((f = fopen(argv[i], "r")) == NULL) {
			fprintf(stderr, "wc: ");
			perror(argv[i]);
			continue;
		}
		count(f, argv[i]);
		fclose(f);
	}
	if (nfiles > 1)
		report(tlines, twords, tchars, "total");
	exit(0);
}

count(f, name)
FILE *f;
char *name;
{
	register int c;
	register char inword;
	long lines, words, chars;

	lines = words = chars = 0;
	inword = 0;
	while ((c = getc(f)) != EOF) {
		chars++;
		if (c == '\n')
			lines++;
		if (c == ' ' || c == '\t' || c == '\n')
			inword = 0;
		else if (!inword) {
			inword = 1;
			words++;
		}
	}
	tlines += lines;
	twords += words;
	tchars += chars;
	report(lines, words, chars, name);
}

report(lines, words, chars, name)
long lines, words, chars;
char *name;
{
	if (lflag)
		printf("%7ld", lines);
	if (wflag)
		printf("%7ld", words);
	if (cflag)
		printf("%7ld", chars);
	if (name)
		printf(" %s", name);
	printf("\n");
}
