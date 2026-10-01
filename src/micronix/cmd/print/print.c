/*
 * print - paginate files for a printer
 *
 * cmd/print/print.c
 *
 * A reconstruction of /bin/print off the Micronix 1.6 distribution -
 * inode 26 of 1012-8_dist_2.IMD - from its disassembly.  There is no
 * source for it.  README says how it was read.  It is a pr-like
 * formatter and not the spooler: it writes pages to the standard output
 * and leaves it to something else to get them to a printer.
 *
 * print [-N] [-lN] [-wN] [file ...]
 *
 *	-N	N columns across the page; the default is 1
 *	-lN	N lines to a page, the default 66
 *	-wN	the page is N columns wide, the default 79
 *
 * Every page is five lines of heading - two blank lines, a line of
 * "name Page n date", and two more blank lines - the body, and four
 * blank lines.  The body is the page length less nine lines.  Each
 * file is numbered from page 1 under its own name; with no file the
 * standard input is read and the name is empty.  The date is the time
 * the command started, and is the same on every page.
 *
 * Lines are not truncated, they are folded.  A line is cut into words -
 * a word being the blanks and control characters before it and the run
 * of printable characters after - and as many as fit are put on one
 * piece.  The first piece of a line may be two columns shorter than the
 * others, and a word that is wider than a piece goes on a piece of its
 * own and is cut off at the page width when it is printed.  Pieces,
 * not lines, are what the columns are made of: the pieces fill the
 * first column down, then the second, and so on, and a page holds
 * (length - 9) * N of them.
 *
 * What follows are the places where the original does something that is
 * not what the manual says, and which are kept because this is meant to
 * be what the original was and not what it should have been.
 *
 *	A row of the page that has nothing in it is written as one blank
 *	and a newline, not as a newline.  This is the last page's
 *	padding as much as it is a line with nothing in it.
 *
 *	A partial last page is padded to the full number of rows.
 *
 *	A page length under nine leaves a negative number of rows, and
 *	the original, its allocation refused, stopped without a word and
 *	with a status of 1.  So does this.
 *
 *	A line longer than 511 characters is read as pieces of that
 *	length, as the original's getl was handed 512 and no more, and
 *	each is a line of its own.
 *
 *	Tabs inside a piece are expanded against the start of the piece
 *	and not of the line.
 *
 * What is not kept: it converted the numbers in the options with a
 * library routine that was told the string was 512 characters long and
 * stopped at the end of that - which, with the arguments at the very top
 * of memory, as they are in this system, wraps the pointer and converts
 * nothing, so that -N, -lN and -wN each gave a value of 0 and the usage
 * message.  They work here.
 *
 * The original ended with whatever exit left in BC, except for the
 * standard input, where it was 1.  This exits 0 when it has printed and
 * 1 when it has not.  The messages are the original's: the usage line
 * on the standard output, and the others on the standard error.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

#define	LINESZ		512
#define	ROWSZ		512

int	pagelen = 66;		/* -l */
int	ncols = 1;		/* -N */
int	width = 79;		/* -w */

int	nfiles;
int	colw;			/* width of a column: width / ncols */
int	body;			/* rows to a page: pagelen - 9 */
int	ncells;			/* pieces to a page: body * ncols */
int	page;
int	fill;
char	**cells;
char	*fname = "";
char	*date;
long	now;

/*
 * The line, the word cut from it and the piece being built are three
 * buffers one after another, as they are in the original, and that is
 * not an accident of the layout that can be tidied: a line of exactly
 * LINESZ characters has no room for its terminator and the NUL goes
 * into the first byte of the word, so the scan carries on from the end
 * of the line into the last word it cut and cuts it again.  The
 * original's output has that word twice, and so does this.
 */
char	area[3 * LINESZ + 1];
#define	line	(area)
#define	word	(area + LINESZ)
#define	piece	(area + 2 * LINESZ)
char	row[ROWSZ];

usage()
{
	fputs("usage: print [-l#] [-w#] [-#] [file ... ]\n", stdout);
	fflush(stdout);
	exit(1);
}

nomem()
{
	perror("pr");
	exit(1);
}

/*
 * A string of digits, the empty string included.  (08a8)
 */
numeric(s)
	register char *s;
{
	for (; *s; s++)
		if (*s < '0' || *s > '9')
			return 0;
	return 1;
}

/*
 * The options.  A word that begins with a dash is an option and the
 * rest are files.  (0207)
 */
getargs(argc, argv)
	int argc;
	char *argv[];
{
	register int i;
	register char *s;

	for (i = 1; i < argc; i++) {
		s = argv[i];
		if (*s != '-') {
			nfiles++;
			continue;
		}
		s++;
		if (numeric(s))
			ncols = atoi(s);
		else if (*s == 'l' && numeric(s + 1))
			pagelen = atoi(s + 1);
		else if (*s == 'w' && numeric(s + 1))
			width = atoi(s + 1);
		else {
			write(2, argv[i], strlen(argv[i]));
			write(2, ": Unknown argument\n", 19);
			exit(1);
		}
	}
	if (ncols == 0 || pagelen == 0)
		usage();
	colw = width / ncols;
	body = pagelen - 9;
	ncells = body * ncols;
	if (ncells < 0)
		exit(1);
	time(&now);
	date = ctime(&now);
	cells = (char **)calloc(ncells > 0 ? ncells + 1 : 1, sizeof(char *));
	if (cells == NULL)
		nomem();
}

/*
 * The width a string takes up: a tab goes to the next multiple of
 * eight, a backspace takes one off, a newline nothing.  (0be8)
 */
swidth(s)
	register char *s;
{
	register int w;

	w = 0;
	for (; *s; s++) {
		if (*s == '\b')
			w--;
		else if (*s == '\t')
			w = (w + 8) & ~7;
		else if (*s != '\n')
			w++;
	}
	return w;
}

/*
 * Write n newlines.  (082a)
 */
blanks(n)
	register int n;
{
	while (n-- > 0)
		putchar('\n');
}

/*
 * Blanks and tabs off the front.  (0c73)
 */
char *
skipblank(s)
	register char *s;
{
	while (*s == ' ' || *s == '\t')
		s++;
	return s;
}

/*
 * Split off a word: the blanks and control characters, then the
 * printable characters after them.  (0b50)
 *
 * The copy is stopped at LINESZ - 1 characters.  The original's was
 * not, and a line of 512 blanks or of 512 characters with none between
 * them sent it round for ever, the word it was writing being the
 * buffer it was reading from; the scan still goes to the end of the
 * word, and the word is cut short.
 */
char *
getword(s, w)
	register char *s, *w;
{
	register char *e;

	e = w + LINESZ - 1;
	while (*s && (*s <= ' ' || *s >= 0177)) {
		if (w < e)
			*w++ = *s;
		s++;
	}
	while (*s && *s > ' ' && *s < 0177) {
		if (w < e)
			*w++ = *s;
		s++;
	}
	*w = '\0';
	return s;
}

/*
 * Copy a piece into a row, expanding tabs against the start of it.
 * (09a5)
 */
putcell(s, d)
	register char *s, *d;
{
	register int i;

	i = 0;
	while (*s) {
		if (*s == '\t') {
			s++;
			*d++ = ' ';
			i++;
			while (i & 7) {
				*d++ = ' ';
				i++;
			}
		} else if (*s == '\b') {
			*d++ = *s++;
			if (i)
				i--;
		} else {
			*d++ = *s++;
			i++;
		}
	}
}

/*
 * Write the page that has been gathered, if there is one.  The pieces
 * are in order, and the column k begins at piece k * body.  (0625)
 */
flush()
{
	register int r, k;
	register char *p;
	char *q;

	if (cells[0] == NULL)
		return;

	blanks(2);
	page++;
	printf("%s Page %d %s", fname, page, date);
	blanks(2);

	for (r = 0; r < body; r++) {
		memset(row, ' ', width + 2);
		for (k = 0; k < ncols; k++) {
			q = cells[k * body + r];
			if (q != NULL && *q != '\0')
				putcell(q, row + k * colw);
		}
		p = row + width;
		while (p > row && *p == ' ')
			p--;
		p[1] = '\n';
		p[2] = '\0';
		fputs(row, stdout);
	}

	blanks(4);

	for (k = 1; k <= ncells; k++) {
		if (cells[k - 1] != NULL) {
			free(cells[k - 1]);
			cells[k - 1] = NULL;
		}
	}
	fill = 0;
}

/*
 * Keep a piece.  When the page is full, write it.  (05d2)
 */
addcell(s)
	char *s;
{
	char *p;

	p = malloc(strlen(s) + 1);
	if (p == NULL)
		nomem();
	strcpy(p, s);
	cells[fill++] = p;
	if (fill >= ncells) {
		flush();
		fill = 0;
	}
}

/*
 * A line, folded into pieces.  The room for a word is what is left of
 * the column, two less for the first piece of a line.  (0410)
 */
fold()
{
	register char *p, *e;
	register int rem, n;
	char *w;

	if (line[0] == '\0') {
		addcell(line);
		return;
	}
	p = line;
	rem = colw - 2;
	n = 0;
	e = piece;
	*e = '\0';
	for (;;) {
		p = getword(p, word);
		if (word[0] == '\0') {
			if (n)
				addcell(piece);
			return;
		}
		if (swidth(word) < rem) {
			e = strcpy(e, word) + strlen(word);
			rem -= swidth(word);
			n++;
			continue;
		}
		if (n)
			addcell(piece);
		w = skipblank(word);
		rem = colw - swidth(w);
		strcpy(piece, w);
		e = piece + strlen(piece);
		n = 1;
	}
}

/*
 * Read up to LINESZ characters, as far as a newline.  (1b09)
 */
getl(fp, s, max)
	FILE *fp;
	register char *s;
	int max;
{
	register int c, n;

	n = 0;
	while (n < max && (c = getc(fp)) != EOF) {
		s[n++] = c;
		if (c == '\n')
			break;
	}
	return n;
}

/*
 * One file, or the standard input.  (03e6, 0410)
 */
printfile(fp)
	FILE *fp;
{
	register int n;

	page = 0;
	fill = 0;
	while ((n = getl(fp, line, LINESZ)) > 0) {
		line[n] = '\0';
		while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n'))
			line[--n] = '\0';
		fold();
	}
	flush();
}

main(argc, argv)
	int argc;
	char *argv[];
{
	register int i;
	FILE *fp;

	getargs(argc, argv);
	if (nfiles == 0) {
		printfile(stdin);
		exit(0);
	}
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			continue;
		fp = fopen(argv[i], "r");
		if (fp == NULL) {
			perror(argv[i]);
			exit(1);
		}
		fname = argv[i];
		printfile(fp);
		fclose(fp);
	}
	exit(0);
}
