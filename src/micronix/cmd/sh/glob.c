/*
 * glob.c - filename patterns
 *
 * micronix/cmd/sh/glob.c
 *
 * "No match." at 0x02cb in the image is what told us this was here at
 * all; it turned up while reading the tokeniser and nothing else in
 * NOTES had mentioned it.  Everything below was then read by running
 * the image beside this - see NOTES for the table.
 *
 * The language is three things:
 *
 *	*	any run of characters, including none
 *	?	exactly one character
 *	[...]	one of these, and a-z is a range
 *
 * with two absences worth writing down, because both are what a
 * modern shell would do and neither is what this one does.  There is
 * no negation: "[!bg]*" in /etc answers banner and group, so the "!"
 * is just another character in the set.  And a leading dot is not
 * special: "/*" answers /.login and /.sh along with the rest.
 *
 * A pattern applies to every component of a path and not only the
 * last, so "/e*'/'passwd" finds /etc/passwd.  What comes back is
 * sorted - the image answers /etc/* in alphabetical order where the
 * directory itself holds ttys, group, init, signon ... - so the names
 * from each directory are sorted as they are read.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/dir.h>
#include "sh.h"

#define DIRSIZ  14              /* sys/dir.h: char name[14] */
#define NAMESZ  (DIRSIZ + 1)    /* and room to terminate it */

/*
 * The path being built up.  One buffer rather than one per level of
 * recursion: this runs on a machine with 64k in it, and a local of
 * this size at every step of a path is not the way to spend it.  Each
 * step appends, recurses, and puts the length back.
 */
static char path[MAXPATH];

/*
 * Does this word want expanding at all?  A word with none of these in
 * it is not a pattern, is not looked for on the disk, and cannot fail
 * to match - "echo plain" is the word plain whether or not any such
 * file exists.
 */
int
ispattern(s)
char *s;
{
	while (*s) {
		if (*s == '*' || *s == '?' || *s == '[')
			return 1;
		s++;
	}
	return 0;
}

/*
 * "." and ".." and nothing else.
 *
 * A leading dot is NOT special here - the image answers "/*" with
 * /.login and /.sh among the rest, where a modern shell would hide
 * them - but these two are still skipped, or every pattern would
 * answer with the directory it was looking in.
 */
static int
dotdir(name)
char *name;
{
	if (name[0] != '.')
		return 0;
	if (name[1] == '\0')
		return 1;
	return name[1] == '.' && name[2] == '\0';
}

/*
 * Match one path component against one pattern component.
 *
 * The recursion is on "*" and goes no deeper than the name is long,
 * which is fourteen.
 */
static int
match(p, s)
char *p;
char *s;
{
	int lo;
	int ok;

	while (*p) {
		if (*p == '*') {
			p++;
			if (!*p)
				return 1;		/* a trailing * takes the rest */
			while (*s) {
				if (match(p, s))
					return 1;
				s++;
			}
			return match(p, s);	/* and the empty tail */
		}
		if (!*s)
			return 0;
		if (*p == '?') {
			p++;
			s++;
			continue;
		}
		if (*p == '[') {
			p++;
			ok = 0;
			while (*p && *p != ']') {
				lo = *p;
				if (p[1] == '-' && p[2] && p[2] != ']') {
					if (*s >= lo && *s <= p[2])
						ok = 1;
					p += 3;
				} else {
					if (*s == lo)
						ok = 1;
					p++;
				}
			}
			if (*p == ']')
				p++;
			if (!ok)
				return 0;
			s++;
			continue;
		}
		if (*p != *s)
			return 0;
		p++;
		s++;
	}
	return *s == '\0';
}

/*
 * Every name in the directory that path names which matches pat,
 * sorted, as one block of NAMESZ-byte slots.  *n is how many.
 *
 * Read twice: once to count and once to fill.  The alternative is to
 * guess how many a directory holds and be wrong in one of the two
 * directions, and this is a shell that has to run on a floppy where
 * being wrong the generous way costs real memory.
 */
static char *
names(plen, pat, n)
int plen;
char *pat;
int *n;
{
	struct dir d;
	char nm[NAMESZ];
	char tmp[NAMESZ];
	FILE *f;
	char *v;
	char *a;
	char *b;
	int count;
	int i;
	int j;

	path[plen] = '\0';
	f = fopen(plen ? path : ".", "r");
	if (!f)
		return (char *)0;

	count = 0;
	while (fread((char *)&d, sizeof(d), 1, f) == 1) {
		if (d.ino == 0 || dotdir(d.name))
			continue;
		strncpy(nm, d.name, DIRSIZ);
		nm[DIRSIZ] = '\0';
		if (match(pat, nm))
			count++;
	}
	if (count == 0) {
		fclose(f);
		*n = 0;
		return (char *)0;
	}
	if (!(v = malloc(count * NAMESZ))) {
		fclose(f);
		*n = 0;
		return (char *)0;
	}

	rewind(f);
	i = 0;
	while (i < count && fread((char *)&d, sizeof(d), 1, f) == 1) {
		if (d.ino == 0 || dotdir(d.name))
			continue;
		strncpy(nm, d.name, DIRSIZ);
		nm[DIRSIZ] = '\0';
		if (match(pat, nm))
			strcpy(&v[i++ * NAMESZ], nm);
	}
	fclose(f);

	/*
	 * The directory is in whatever order it was written in and the
	 * image answers in alphabetical order, so sort.  A selection
	 * sort over a handful of names, swapping the slots themselves -
	 * no second array of pointers to pay for.
	 */
	for (i = 0; i < count - 1; i++) {
		for (j = i + 1; j < count; j++) {
			a = &v[i * NAMESZ];
			b = &v[j * NAMESZ];
			if (strcmp(b, a) < 0) {
				strcpy(tmp, a);
				strcpy(a, b);
				strcpy(b, tmp);
			}
		}
	}
	*n = count;
	return v;
}

/*
 * Expand what is left of a pattern against the directory that the
 * first plen characters of path name, and add what it finds to c.
 *
 * Returns how many names were added, or -1 if there was no room for
 * them.  A component with nothing special in it is appended without
 * asking the disk whether it is there; only a component with a
 * pattern in it causes a directory to be read, which is why
 * "/nosuch/passwd" is a word and "/nosuch/*" is a failure to match.
 */
static int
expand(plen, pat, c)
int plen;
char *pat;
struct cmd *c;
{
	char comp[MAXPATH];
	char *v;
	char *rest;
	char *q;
	int n;
	int i;
	int got;
	int r;

	/*
	 * A leading slash is part of the path and not a component of
	 * its own; so is a run of them between components.
	 */
	while (*pat == '/') {
		if (plen < MAXPATH - 1)
			path[plen++] = '/';
		pat++;
	}
	if (!*pat) {
		/*
		 * The pattern ended on a slash - "/etc/*'/'".  The image
		 * drops it, so there is nothing more to match and what we
		 * have is the answer.
		 */
		if (plen > 1 && path[plen - 1] == '/')
			plen--;
		path[plen] = '\0';
		return addmatch(path, c);
	}

	q = comp;
	while (*pat && *pat != '/')
		*q++ = *pat++;
	*q = '\0';
	rest = pat;

	if (!ispattern(comp)) {
		n = strlen(comp);
		if (plen + n >= MAXPATH)
			return 0;
		strcpy(&path[plen], comp);
		plen += n;
		if (*rest)
			return expand(plen, rest, c);
		path[plen] = '\0';
		return addmatch(path, c);
	}

	if (!(v = names(plen, comp, &n)))
		return 0;

	got = 0;
	for (i = 0; i < n; i++) {
		q = &v[i * NAMESZ];
		if (plen + strlen(q) >= MAXPATH)
			continue;
		strcpy(&path[plen], q);
		if (*rest)
			r = expand(plen + strlen(q), rest, c);
		else {
			path[plen + strlen(q)] = '\0';
			r = addmatch(path, c);
		}
		if (r < 0) {
			free(v);
			return -1;
		}
		got += r;
	}
	free(v);
	return got;
}

/*
 * Expand one word onto the end of a command.
 *
 * Returns how many names it came to.  ZERO IS NOT AN ERROR HERE: a
 * pattern that matches nothing contributes nothing and the caller
 * decides what to say about it, because the image only complains when
 * NOTHING in the whole statement matched - "echo /nosuch/* /etc/pass*"
 * prints /etc/passwd and says not a word about the first.
 */
int
globword(w, c)
char *w;
struct cmd *c;
{
	return expand(0, w, c);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
