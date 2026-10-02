/*
 * include - file inclusion
 *
 * cmd/include/include.c
 *
 * A reconstruction of /usr/bin/include off the Micronix 1.6
 * distribution - inode 25 of 1013-8_dist_3.IMD - from its disassembly.
 * There is no source for it.  README says how it was read.  It is the
 * program of that name in Kernighan and Plauger's Software Tools.
 *
 * A filter: the standard input is copied to the standard output, except
 * that a line that begins
 *
 *	include filename
 *
 * is replaced by the contents of the file, which may have such lines in
 * it in turn.  "Begins" is not quite so: it is the first word on the
 * line, and the line may be indented.  The file name is the next word
 * and the rest of the line is not read.  An "include" with no name is
 * dropped.  A file that will not open is the end of the run, and the
 * name and ": can't read" are written to the standard error.
 *
 * What it does that the manual does not say:
 *
 *	A line is read the way fgets reads one, 511 characters at the
 *	most, and a longer line is two lines - so a line of 600
 *	characters whose second piece begins with "include" is an
 *	include.
 *
 *	The last line of a file that has no newline at the end is lost.
 *	The test for the end of the file is made after the line has been
 *	read, and a line that ran into the end of the file has already
 *	set it.  So it is not written, and an include on it is not done.
 *
 *	The two buffers, the line and the word, are shared by all the
 *	levels of an include.  Nothing is the worse for it.
 *
 * The original exited 1 for a good run and 0 for a file it could not
 * open.  It is 0 and 1 here.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

#define	LINESZ	512
#define	MAXOPEN	13

char	line[LINESZ];
char	word[LINESZ];

/*
 * A file being read: a descriptor and a buffer, a byte at a time.  The
 * original had sixteen streams of the library's to be had, three of them
 * the standard ones, so thirteen files can be open at once and an
 * include that would make a fourteenth is a file that will not open.
 * This library has twelve streams, and so the count is kept here.
 */
struct in {
	int fd;
	int n;
	int pos;
	char buf[LINESZ];
};

int	nopen;

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
 * A line of at most n - 1 characters, newline included, and its end
 * - the NUL after it.  Whether the file ended is the answer, and it
 * is so if the end was met at all, in the middle of the line or at
 * the start.  (0712)
 */
getl(ip, s, n)
	struct in *ip;
	register char *s;
	register int n;
{
	register int c;

	while (--n > 0) {
		c = rdc(ip);
		if (c == EOF) {
			*s = '\0';
			return 1;
		}
		*s++ = c;
		if (c == '\n')
			break;
	}
	*s = '\0';
	return 0;
}

/*
 * The first word of s - blanks and control characters skipped, then
 * the printable ones - into w; where it ended is the answer.  (0225)
 */
char *
getword(s, w)
	register char *s, *w;
{
	while (*s && (*s <= ' ' || *s >= 0177))
		s++;
	while (*s && *s > ' ' && *s < 0177)
		*w++ = *s++;
	*w = '\0';
	return s;
}

cantread(name)
	char *name;
{
	write(2, name, strlen(name));
	write(2, ": can't read\n", 13);
	exit(1);
}

/*
 * One file, or the standard input if there is no name.  (012f)
 */
include(name)
	char *name;
{
	struct in *ip;
	register char *p;

	if (name != NULL && *name == '\0')
		return;
	ip = (struct in *)malloc(sizeof *ip);
	if (ip == NULL)
		cantread(name ? name : "stdin");
	ip->n = ip->pos = 0;
	if (name == NULL)
		ip->fd = 0;
	else {
		if (nopen >= MAXOPEN || (ip->fd = open(name, 0)) < 0)
			cantread(name);
		nopen++;
	}
	for (;;) {
		if (getl(ip, line, LINESZ)) {
			if (name != NULL) {
				close(ip->fd);
				nopen--;
			}
			free(ip);
			return;
		}
		p = getword(line, word);
		if (strcmp(word, "include") == 0) {
			getword(p, word);
			include(word);
		} else
			fputs(line, stdout);
	}
}

main()
{
	include((char *)0);
	exit(0);
}
