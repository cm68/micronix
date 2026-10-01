/*
 * wc - count lines, words and characters
 * chars, lines, words - the same count, one at a time
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
 * CHARS, LINES AND WORDS ARE THIS PROGRAM UNDER OTHER NAMES.
 *
 * /bin carries three counters of its own, the distribution's, and
 * they are one program here for the reason man and help are: the
 * counting is the same counting.  main() tells them apart by name,
 * the way man does.
 *
 * They are not quite wc, and the differences are the distribution's,
 * read off the three binaries and their pages rather than assumed.
 * Three of them:
 *
 *	the format	one count, printed in twelve columns, where
 *			wc uses seven.  "chars file1 file2" gives
 *			"          24 file1" and a wc would give
 *			"     24 file1".
 *
 *	the total	labelled TOTAL and not total, and printed
 *			only when more than one file was actually
 *			COUNTED - a name that could not be opened
 *			does not earn a total line.  wc counts
 *			names.
 *
 *	the status	always 1, whether the count worked or not,
 *			which is what all three binaries do.  It is
 *			reproduced rather than corrected; a script
 *			that tests it would see the difference.
 *
 * And "-" is the standard input, unnamed, which the three pages
 * promise and the binaries do.  wc does not take it, and does not
 * here either - the flag scan would have to be told to leave it
 * alone, and that is a change to wc rather than to these.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <string.h>

char	lflag;
char	wflag;
char	cflag;
char	anyflag;			/* a flag was given at all */
char	*distname;			/* chars, lines or words; else 0 */

long	tlines, twords, tchars;

main(argc, argv)
int argc;
char *argv[];
{
	register int i;
	int nfiles;			/* names given - wc's total rule */
	int ncounted;			/* files actually read */
	FILE *f;
	char *p;
	char *prog;

	/*
	 * Which name we were called by.  The basename, since argv[0]
	 * is a path when the shell has to look for us.
	 */
	prog = argv[0];
	for (p = prog; *p; p++)
		if (*p == '/')
			prog = p + 1;
	if (strcmp(prog, "chars") == 0) {
		distname = "chars";
		cflag = 1;
	} else if (strcmp(prog, "lines") == 0) {
		distname = "lines";
		lflag = 1;
	} else if (strcmp(prog, "words") == 0) {
		distname = "words";
		wflag = 1;
	}
	if (distname)
		anyflag = 1;		/* they take no flags, and need none */

	argc--;
	argv++;
	/*
	 * No flag scan under the other three names: they have no flags,
	 * so "-x" is a file called -x and not a complaint.  "-" never
	 * reached this scan anyway - argv[0][1] is the NUL - and is
	 * dealt with in the loop.
	 */
	if (!distname && argc > 0 && argv[0][0] == '-' && argv[0][1]) {
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
		exit(distname ? 1 : 0);
	}
	nfiles = argc;
	ncounted = 0;
	for (i = 0; i < argc; i++) {
		if (distname && strcmp(argv[i], "-") == 0) {
			count(stdin, (char *)0);
			ncounted++;
			continue;
		}
		if ((f = fopen(argv[i], "r")) == NULL) {
			if (distname)
				fprintf(stderr, "%s: can't open\n", argv[i]);
			else {
				fprintf(stderr, "wc: ");
				perror(argv[i]);
			}
			continue;
		}
		count(f, argv[i]);
		fclose(f);
		ncounted++;
	}
	if (distname ? (ncounted > 1) : (nfiles > 1))
		report(tlines, twords, tchars, distname ? "TOTAL" : "total");
	exit(distname ? 1 : 0);
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
		printf(distname ? "%12ld" : "%7ld", lines);
	if (wflag)
		printf(distname ? "%12ld" : "%7ld", words);
	if (cflag)
		printf(distname ? "%12ld" : "%7ld", chars);
	if (name)
		printf(" %s", name);
	printf("\n");
}
