/*
 * common - print lines in common
 *
 * cmd/common/common.c
 *
 * A reconstruction of /usr/bin/common off the Micronix 1.6 distribution -
 * inode 62 of 1012-8_dist_2.IMD - from its disassembly.  There is no
 * source for it.  README says how it was read.  It is the program of that
 * name in Kernighan and Plauger's Software Tools - what comm is, in a
 * different order of columns and with the lines' leading blanks taken
 * off.
 *
 * common [-1] [-2] [-3] [-wN] file1 [file2]
 *
 * Two sorted files are merged, a line at a time.  A line that is only in
 * the first file is written in column 1, one that is only in the second
 * in column 2, and one that is in both in column 3; the columns that
 * are not wanted are not written, and a column's place is not given up
 * for it.  With no -1, -2 or -3 all three are wanted.  The second file is
 * the standard input if it is not named.  The columns are as wide as the
 * page, 80 unless -wN says, over the number of columns wanted, and
 * "column" is as many blanks as that.
 *
 * Lines are put out without the blanks and tabs at their beginning, and
 * with the newline they were read with.  They are compared as they are,
 * newline and all, a byte at a time as signed characters, so a line is
 * before its own continuation, and a file that is not in the order this
 * expects is merged as it comes.
 *
 * What it does that the manual does not say:
 *
 *	The "no columns asked for, so all three" test is made after every
 *	argument, a file name among them.  So "common file1 file2 -1"
 *	is all three columns, since file1 came before the -1 and made it
 *	so.
 *
 *	With no file at all it says the usage and goes on, as if its
 *	input were the standard input twice.  The two streams are read
 *	separately, a block at a time, and so share it between them in
 *	blocks of 512.
 *
 *	A line of exactly 512 characters has no room for its end, and
 *	the NUL goes into the first byte of whatever is next: the second
 *	line buffer, for the first, and the word that holds the last
 *	comparison, for the second.  They are beside each other in that
 *	order for that reason.
 *
 *	A byte with the high bit set is not written.  The text is
 *	expected to be ASCII.
 *
 * What was left out: the number of -wN was read by a routine told the
 * string was 512 long, and which with the arguments at the top of
 * memory read none of it, so -wN gave a width of 0.  It works here.
 *
 * The original exited with whatever was left in BC; this exits 0, and 1
 * for a file that will not open.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

#define	LINESZ	512
#define	USAGE	"usage: common [-1] [-2] [-3] [-wN] file1 [file2]\n"

/*
 * A stream: a descriptor and a buffer of 512, as the library's was.
 */
struct in {
	int fd;
	int n;
	int pos;
	char buf[LINESZ];
};

struct in	in1, in2;

/*
 * The two lines, one after the other, and then the result of the last
 * comparison - which is 0 at the start, and which it is the first byte
 * of that the NUL of a line of 512 characters is written over, so
 * that the 512th character of the second line is followed by what is
 * left of the result: the line that is written is the line and then the
 * character of its own difference.  It is in the original's order
 * and it is kept.
 */
struct {
	char b1[LINESZ];
	char b2[LINESZ];
	int res;
} m;
#define	line1	(m.b1)
#define	line2	(m.b2)
#define	cmp	(m.res)

int	col1, col2, col3;	/* the columns wanted */
int	width = 80;		/* the page */
int	colw = 26;		/* a column: width / the number wanted */
char	*file1, *file2;

die(a, b)
	char *a, *b;
{
	if (a)
		write(2, a, strlen(a));
	write(2, b, strlen(b));
	exit(1);
}

cantopen(name)
	char *name;
{
	die(name, ": can't open\n");
}

rdc(ip)
	register struct in *ip;
{
	if (ip->pos >= ip->n) {
		ip->n = read(ip->fd, ip->buf, LINESZ);
		ip->pos = 0;
		if (ip->n <= 0) {
			ip->n = 0;
			return EOF;
		}
	}
	return ip->buf[ip->pos++] & 0377;
}

/*
 * A line of at most 512 characters, newline included; how many, and not
 * ended.  (07fc)
 */
getl(ip, s)
	struct in *ip;
	register char *s;
{
	register int c, n;

	n = 0;
	while (n < LINESZ && (c = rdc(ip)) != EOF) {
		s[n++] = c;
		if (c == '\n')
			break;
	}
	s[n] = '\0';
	return n;
}

/*
 * Which line is first, and 1 and -1 if the first and the second is
 * empty, the end of its file.  (019f)
 */
compare(s1, s2)
	register char *s1, *s2;
{
	register int a, b;

	if (*s1 == '\0')
		return 1;
	if (*s2 == '\0')
		return -1;
	while (*s1 == *s2) {
		if (*s1 == '\0')
			return 0;
		s1++;
		s2++;
	}
	a = *s1;
	b = *s2;
	if (a > 127)
		a -= 256;
	if (b > 127)
		b -= 256;
	return a - b;
}

/*
 * A line without the blanks and tabs in front of it.  A character with
 * the high bit set is not written: the original passed the character to
 * putc widened as a signed char, and a negative number to putc is the
 * request to flush.  (04f2)
 */
put(s)
	register char *s;
{
	while (*s == ' ' || *s == '\t')
		s++;
	for (; *s; s++)
		if (!(*s & 0200))
			putchar(*s);
}

blanks(n)
	register int n;
{
	while (n-- > 0)
		putchar(' ');
}

/*
 * What to write for the lines read.  (0236)
 */
output()
{
	if (cmp < 0) {
		if (col1) {
			put(line1);
			return;
		}
	}
	if (cmp > 0) {
		if (col2) {
			if (col1)
				blanks(colw);
			put(line2);
		}
		return;
	}
	if (cmp != 0)
		return;
	if (!col3)
		return;
	if (col1)
		blanks(colw);
	if (col2)
		blanks(colw);
	put(line1);
}

/*
 * Convert the digits at the front of a string.
 */
number(s)
	register char *s;
{
	register int n;

	n = 0;
	while (*s >= '0' && *s <= '9')
		n = n * 10 + *s++ - '0';
	return n;
}

options(argc, argv)
	int argc;
	char *argv[];
{
	register int i;
	register char *a;

	for (i = 1; i < argc; i++) {
		a = argv[i];
		if (strcmp(a, "-1") == 0)
			col1 = 1;
		else if (strcmp(a, "-2") == 0)
			col2 = 1;
		else if (strcmp(a, "-3") == 0)
			col3 = 1;
		else if (strncmp(a, "-w", 2) == 0)
			width = number(a + 2);
		else if (file1 == NULL)
			file1 = a;
		else if (file2 == NULL)
			file2 = a;
		if (!col1 && !col2 && !col3)
			col1 = col2 = col3 = 1;
		colw = width / (col1 + col2 + col3);
	}
	if (file1 == NULL)
		write(2, USAGE, strlen(USAGE));
}

main(argc, argv)
	int argc;
	char *argv[];
{
	options(argc, argv);
	if (file1 != NULL && (in1.fd = open(file1, 0)) < 0)
		cantopen(file1);
	if (file2 != NULL && (in2.fd = open(file2, 0)) < 0)
		cantopen(file2);
	for (;;) {
		if (!(cmp > 0))
			getl(&in1, line1);
		if (!(cmp < 0))
			getl(&in2, line2);
		if (line1[0] == '\0' && line2[0] == '\0')
			break;
		cmp = compare(line1, line2);
		output();
	}
	exit(0);
}
