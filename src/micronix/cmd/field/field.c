/*
 * field - pick selected fields from each line of the input
 *
 * cmd/field/field.c
 *
 * Reads the input line by line, breaks each line into fields, and
 * prints the fields named by the arguments, in the order named.  The
 * first field is number 0.  The default input separator is whitespace
 * (runs collapse, but a leading run means field 0 is empty); -t names
 * a single separator character instead, whose every occurrence is a
 * boundary so that "a::b" is three fields.  The output separator is a
 * tab, or the -t character when one is given.
 *
 * A line is at most 512 bytes, as the page says.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <stdlib.h>

#define	MAXLINE	512
#define	MAXFLDS	64

char	line[MAXLINE];
char	*fields[MAXFLDS];
char	sep;			/* separator, or 0 for whitespace */

static int
splitline()
{
	register char *p;
	register int n;
	char *start;

	n = 0;
	p = line;

	if (sep != 0) {
		/* every separator is a boundary; "a::b" is three fields */
		start = p;
		while (*p != '\0') {
			if (*p == sep) {
				*p = '\0';
				fields[n++] = start;
				if (n == MAXFLDS)
					break;
				start = p + 1;
			}
			p++;
		}
		fields[n++] = start;
	} else {
		/* whitespace: runs collapse, but a leading run is an
		   empty field 0 */
		if (*p == ' ' || *p == '\t')
			fields[n++] = "";
		while (*p != '\0') {
			if (*p != ' ' && *p != '\t') {
				fields[n++] = p;
				while (*p != ' ' && *p != '\t' && *p != '\0')
					p++;
				if (*p == ' ' || *p == '\t')
					*p++ = '\0';
				continue;
			}
			p++;
		}
	}
	return n;
}

main(argc, argv)
	int argc;
	char **argv;
{
	int want[MAXFLDS];
	int nwant;
	int i, f, n;
	char *p;
	char outsep;

	nwant = 0;
	i = 1;
	if (i < argc && argv[i][0] == '-' && argv[i][1] == 't') {
		sep = argv[i][2];	/* may be '\0' for a bare -t */
		i++;
	}
	for (; i < argc && nwant < MAXFLDS; i++)
		want[nwant++] = atoi(argv[i]);
	if (nwant == 0) {
		fprintf(stderr, "usage: field [-t sep] field-number ...\n");
		exit(1);
	}
	outsep = sep != 0 ? sep : '\t';
	while (fgets(line, MAXLINE, stdin) != NULL) {
		for (p = line; *p != '\0'; p++)
			if (*p == '\n') {
				*p = '\0';
				break;
			}
		n = splitline();
		for (f = 0; f < nwant; f++) {
			if (f > 0)
				putchar(outsep);
			if (want[f] >= 0 && want[f] < n)
				fputs(fields[want[f]], stdout);
		}
		putchar('\n');
	}
	exit(0);
}
