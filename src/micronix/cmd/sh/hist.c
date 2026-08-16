/*
 * hist.c - "!", the history reference
 *
 * micronix/cmd/sh/hist.c
 *
 * "Event not found." at 0x4f9d is what said this was here, with "Line
 * too long." beside it at 0x4f8d.  There is no "history" builtin -
 * the table at 0x9755 has eighteen names and that is not one of them
 * - so a reference is the whole of it: there is no way to ask the
 * shell what it remembers.
 *
 * It is INTERACTIVE ONLY.  Fed from a file the image treats "!e" as a
 * command name and answers "!e: Command not found.", so a script is
 * never rewritten under itself.
 *
 * What a reference is, read by running the image:
 *
 *	!word	the last line that STARTS WITH word
 *
 * and that is all of it.  There are no event numbers - "!1" and "!2"
 * both answer "Event not found." where "!e" finds one, because 1 and
 * 2 are being looked for as prefixes and nothing begins with them.
 * None of csh's other forms are here either: "!!" and "!$" are left
 * alone entirely and run as command names, where "!:" and "!^" and
 * "!#" all go looking and fail.  So the trigger is any character
 * except those two and whitespace, and the word runs to the next
 * space - "!e-tail" looks for e-tail and does not find it.
 *
 * A reference may sit anywhere in the line, not only at the front:
 * "echo !e" becomes "echo echo one".  What goes into the history is
 * the line AFTER expansion, which is why a second "!e" then finds the
 * expanded one.
 *
 * A line with a "!" anywhere in it is printed before it runs,
 * WHETHER OR NOT anything expanded: the image echoes "echo a!$b" and
 * "echo x!!" back just as they were typed, and neither of those has a
 * reference in it.  So the test is the bare character and not whether
 * it came to anything.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include "sh.h"

/*
 * How many lines are kept is not read from the image - there is no
 * builtin that would show it and no message that depends on it.
 *
 * Pointers rather than a table of lines: sixteen of these is
 * thirty-two bytes of bss against sixteen line buffers, and the lines
 * themselves are only as long as they are.  On a machine with 64k the
 * difference is worth the malloc.
 */
#define NHIST   16

static char *hist[NHIST];
static char hnext;                      /* 0..NHIST-1, sixteen */

/*
 * Does a "!" here begin a reference?
 *
 * "!!" and "!$" do not, in the image, and are left to be run as
 * whatever they are; everything else that is not whitespace does.
 */
static int
trigger(c)
char c;
{
	if (c == '\0' || c == ' ' || c == '\t' || c == '\n')
		return 0;
	return c != '!' && c != '$';
}

/*
 * The most recent line beginning with the first len characters of
 * pre, or null.
 */
static char *
find(pre, len)
char *pre;
int len;
{
	int i;
	int n;

	for (n = 0; n < NHIST; n++) {
		i = hnext - 1 - n;
		while (i < 0)
			i += NHIST;
		if (!hist[i])
			continue;
		if (strncmp(hist[i], pre, len) == 0)
			return hist[i];
	}
	return (char *)0;
}

/*
 * Remember a line, as it finally read.  The newline goes; it belongs
 * to the reading and not to the line.
 */
void
histadd(line)
char *line;
{
	char *p;
	int n;

	n = strlen(line);
	while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
		n--;
	if (n == 0)
		return;

	if (hist[hnext])
		free(hist[hnext]);
	if (!(p = malloc(n + 1)))
		return;                     /* remembering is not worth failing over */
	strncpy(p, line, n);
	p[n] = '\0';
	hist[hnext] = p;
	hnext = (hnext + 1) % NHIST;
}

/*
 * Expand the references in a line, in place.
 *
 * Returns 1 if the line held a "!" at all, which is when the caller
 * prints it; 0 if it held none; and -1 if a reference found nothing,
 * having said "Event not found." itself.
 */
int
histexpand(line, size)
char *line;
int size;
{
	char buf[MAXLINE];
	char *s;
	char *q;
	char *w;
	char *found;
	int len;
	int sawbang = 0;

	s = line;
	q = buf;
	while (*s) {
		if (*s != '!' || !trigger(s[1])) {
			if (*s == '!')
				sawbang = 1;    /* enough on its own to echo */
			if (q < buf + sizeof(buf) - 1)
				*q++ = *s;
			s++;
			continue;
		}
		sawbang = 1;

		s++;                        /* past the "!" */
		w = s;
		while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r')
			s++;
		len = s - w;

		if (!(found = find(w, len))) {
			perr("Event not found.");
			return -1;
		}
		while (*found && q < buf + sizeof(buf) - 1)
			*q++ = *found++;
	}
	*q = '\0';

	if (!sawbang)
		return 0;

	/*
	 * The newline came in on the line and was copied along with it;
	 * take it off and put one back, so that a line which expanded
	 * and one which did not both end with exactly one.
	 */
	while (q > buf && (q[-1] == '\n' || q[-1] == '\r'))
		*--q = '\0';

	if ((q - buf) + 2 > size) {
		perr("Line too long.");     /* 0x4f8d, beside the other one */
		return -1;
	}
	strcpy(line, buf);
	strcat(line, "\n");
	return 1;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
