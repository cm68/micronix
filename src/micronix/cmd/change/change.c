/*
 * change - make global changes in a stream
 *
 * cmd/change/change.c
 *
 * A reconstruction of /usr/bin/change off the Micronix 1.6 distribution -
 * inode 63 of 1012-8_dist_2.IMD - from its disassembly.  There is no
 * source for it.  README says how it was read.  It is the program of that
 * name in Kernighan and Plauger's Software Tools, with a different regular
 * expression and with tags.
 *
 * change from [to]
 *
 * The standard input is copied to the standard output with every string
 * that FROM matches replaced by TO, which is nothing if it is not given.
 * What is changed is found a line at a time, and from left to right: at
 * each place in the line the longest match - by the rule that a closure
 * takes as much as it can and gives it back one at a time - is looked
 * for, and if there is one that is not empty it is replaced and the next
 * place looked at is the one after it.  A line does not include its
 * newline in what can be matched, whatever it says.
 *
 * The pattern is made of
 *
 *	c	any other character, itself
 *	@c	the character c, whatever it is, and "@(" and "@)" are
 *		the exceptions
 *	?	any character but the newline
 *	[...]	any of the characters, with a-b for the ones between
 *	[^...]	any character but those
 *	%	the beginning of the line - it is one wherever it is
 *	$	the end of it - the same
 *	x*	any number of x, for any of the above but % and the
 *		tags; a % has the * for itself, and the * is itself at
 *		the start of the pattern
 *	@(...@)	a tag: what it matches can be had again in TO by @1 to
 *		@9, counted from the left of the @(.  Nine of them at
 *		most, and they nest
 *
 * and TO is made of
 *
 *	&	whatever the whole pattern matched
 *	@n	the nth tag, for n from 1 to 9
 *	@c	the character c, for any other
 *	c	anything else, itself
 *
 * so that "change @(a@)@(b@) @2@1" turns every "ab" into "ba".
 *
 * Errors are written to the standard output, one to a line, and are the
 * end of the run.  They are the usage line, "imbalanced parentheses." for
 * an @) with no @( and "too many tagged fields." for a tenth, and
 * "Missing trailing delimiter." for a pattern with a newline in it.  An
 * @( that is not closed is not an error: it is closed at the end.
 *
 * The things that are done as the original does them, and not as might
 * be expected:
 *
 *	The pattern is made by walking the argument, and a [ with no
 *	] after it walks off the end of the argument and on into the
 *	next one - the replacement text, if there is one.  Where it ends
 *	is where that does.  So "change [a X" has a pattern that is
 *	"[aX", and "change [abc" has a pattern that is whatever
 *	follows, and may not end.
 *
 *	A line is read the way fgets reads it, 511 characters at a time,
 *	and each piece is a line, and the last line of a file with no
 *	newline at the end of it is not looked at: the end of the file has
 *	been met by the time the line is read, and the test is made after.
 *
 *	A line made longer by the changes than the buffer for it - 512
 *	- ran over the end of it, and into the address of the head of the
 *	pattern, which is the next thing, and then the pattern was
 *	whatever that address now pointed at: the original did nothing
 *	more to such a line, or any after it, that is predictable.  The
 *	buffer is 4096 here, and it is enough for the longest line the
 *	changes can make of 511.
 *
 * "illegal from pattern." and "illegal to pattern." are what the original
 * says when what makpat or makesub gives back is -3.  They give back where
 * they stopped, which is an address, and -3 is the address 0177775: so a
 * pattern or a replacement that ends exactly there is an error, which is
 * to say that whether "change ? ''" is one depends on how long the
 * arguments before and after are.  It is kept, because the arguments
 * are laid out here as they are there.
 *
 * The exit status was 0 for an error and whatever was in BC for a good
 * run.  It is 1 and 0 here.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>

#define	LINESZ	512
#define	OUTSZ	4096
#define	NTAGS	9
#define	SC(x)	((x) > 127 ? (x) - 256 : (x))	/* a char as the original had it: signed */

/*
 * The pattern is a list of nodes.  The type is 'a' for a character,
 * '?' for any, '[' for a set and ']' for a set that is the opposite,
 * '%' and '$' for the ends of a line, and '{' and '}' for the start
 * and the end of a tag, whose number is in spec.
 */
struct node {
	struct node *next;
	char type;
	char *set;
	int closure;
	char spec;
};

/*
 * The tags are the addresses in the line of the start and the end of what
 * each matched, or 0.
 */
struct node	*head;
char	*tagstart[NTAGS + 1];
char	*tagend[NTAGS + 1];
char	line[LINESZ];
char	out[OUTSZ];
char	setbuf[LINESZ];
char	sub[LINESZ];
char	*lineptr;

/*
 * An error: what is wrong, and the end.  (07eb)
 */
fatal(msg)
	char *msg;
{
	fputs(msg, stdout);
	fputs("\n", stdout);
	fflush(stdout);
	exit(1);
}

/*
 * Whether a character is in a set.  (02be)
 */
inset(c, s)
	register int c;
	register char *s;
{
	for (; *s; s++)
		if (*s == c)
			return 1;
	return 0;
}

/*
 * A character into a set, if it is not there already.  (06db)
 */
addset(c, s)
	int c;
	register char *s;
{
	if (inset(c, s))
		return;
	while (*s)
		s++;
	*s++ = c;
	*s = '\0';
}

/*
 * The set that a [ begins, from there to the ], in set.  A range is
 * a-b; the character after an @ is itself; a - at either end, or
 * next to the ], is itself.  (04c5)
 */
mkset(p, set)
	register char *p;
	register char *set;
{
	register int c, lo, hi;
	char *start;

	*set = '\0';
	if (*p == '[')
		p++;
	if (*p == '^')
		p++;
	start = p;
	for (;;) {
		c = *p;
		if (c == '\0')
			return;
		if (c == '@') {
			if (p[1] == '\0' || p[1] == ']')
				addset('@', set);
			else {
				p++;
				addset(*p, set);
			}
		} else if (c == '-') {
			if (p == start || p[1] == '\0' || p[1] == ']')
				addset('-', set);
			else {
				lo = SC(p[-1] & 0377);
				if (p[1] == '@')
					hi = SC(p[2] & 0377);
				else
					hi = SC(p[1] & 0377);
				for (; lo <= hi; lo++)
					addset(lo, set);
				p++;
				if (*p == '@')
					p++;
			}
		} else if (c == ']')
			return;
		else
			addset(c, set);
		p++;
	}
}

/*
 * One more node, or the closure on the last.  (030b)
 */
addnode(type, spec)
	register int type;
	char *spec;
{
	register struct node *n;

	n = head;
	while (n->type != 0 && n->next != NULL && n->next->type != 0)
		n = n->next;
	if (type == '*') {
		if (n->type == '%') {
			addnode('a', (char *)'*');
			return;
		}
		n->closure = 1;
		return;
	}
	if (n->type != 0) {
		if (n->next == NULL)
			n->next = (struct node *)calloc(8, 1);
		n = n->next;
	}
	n->type = type;
	n->spec = (int)spec;
	n->closure = 0;
	if (n->set != NULL) {
		free(n->set);
		n->set = NULL;
	}
	if (n->type == '[') {
		if (spec[1] == '^')
			n->type = ']';
		mkset(spec, setbuf);
		n->set = (char *)malloc(strlen(setbuf) + 1);
		strcpy(n->set, setbuf);
	}
	if (n->next != NULL)
		n->next->type = 0;
}

/*
 * The pattern, from its text up to delim, into the list.  Returns where
 * it stopped.  (0b08)
 */
char *
makpat(arg, delim)
	char *arg;
	int delim;
{
	register char *p;
	int ntags, sp;
	char stack[NTAGS];
	register int c;

	ntags = 0;
	sp = 0;
	n_init();
	for (p = arg; *p != delim; p++) {
		c = *p;
		if (c == '\0')
			break;
		if (c == '@') {
			if (p[1] == '\0') {
				addnode('a', (char *)'@');
				continue;
			}
			p++;
			c = *p;
			if (c == '(') {
				ntags++;
				if (ntags > NTAGS)
					fatal("too many tagged fields.");
				stack[sp++] = ntags;
				addnode('{', (char *)ntags);
			} else if (c == ')') {
				sp--;
				if (sp < 0)
					fatal("imbalanced parentheses.");
				addnode('}', (char *)stack[sp]);
			} else
				addnode('a', (char *)c);
		} else if (c == '%')
			addnode('%', (char *)0);
		else if (c == '$')
			addnode('$', (char *)0);
		else if (c == '?')
			addnode('?', (char *)0);
		else if (c == '[') {
			addnode('[', p);
			while (*p != '\0' && *p != ']')
				p++;
		} else if (c == '*') {
			if (p == arg)
				addnode('a', (char *)'*');
			else
				addnode('*', (char *)0);
		} else if (c == '\n')
			break;
		else
			addnode('a', (char *)c);
	}
	if (*p != delim)
		fatal("Missing trailing delimiter.");
	return p;
}

n_init()
{
	head = (struct node *)calloc(8, 1);
}

/*
 * The replacement into a string, and its end.  & is 0366 and a tag
 * reference 0377 to 0367; the rest is the text.  (0f84)
 */
char *
makesub(arg, dst, delim)
	register char *arg, *dst;
	int delim;
{
	register int c;

	for (; *arg != '\0' && *arg != delim; arg++) {
		c = *arg;
		if (c == '@') {
			if (arg[1] == '\0' || arg[1] == delim)
				*dst++ = '@';
			else {
				arg++;
				c = *arg;
				if (c >= '1' && c <= '9')
					*dst++ = '0' - c;
				else
					*dst++ = c;
			}
		} else if (c == '&')
			*dst++ = 0366;
		else
			*dst++ = c;
	}
	*dst = '\0';
	if (*arg != delim)
		fatal("Missing trailing delimiter.");
	return arg;
}

/*
 * Whether a character is matched by a node that is one.  The end of
 * the string is matched by none.  (0e25)
 */
omatch(p, n)
	register char *p;
	register struct node *n;
{
	if (*p == '\0')
		return 0;
	switch (n->type) {
	case 'a':
		return n->spec == *p;
	case '?':
		return *p != '\n';
	case '[':
		return inset(*p, n->set);
	case ']':
		return !inset(*p, n->set) && *p != '\n';
	case '%':
		return 1;
	case '$':
		return *p == '\n';
	}
	return 0;
}

/*
 * The end of the match of the pattern from n, at p, or 0.  A match that
 * went up to the end of the string is said to end at its last character,
 * which is how a match that includes the newline stops short of it.
 * (10fc)
 */
char *
amatch(p, n)
	register char *p;
	register struct node *n;
{
	register char *q, *r;

	r = 0;
	for (; n != NULL && n->type != 0; p++, n = n->next) {
		if (n->closure) {
			for (q = p; omatch(q, n); q++)
				;
			for (;;) {
				if (q < p)
					return 0;
				r = amatch(q, n->next);
				if (r != 0)
					return r;
				q--;
			}
		}
		if (n->type == '{') {
			tagstart[n->spec] = p;
			p--;
		} else if (n->type == '}') {
			tagend[n->spec] = p;
			p--;
		} else if (n->type == '%') {
			if (p != lineptr)
				return 0;
			p--;
		} else if (!omatch(p, n))
			return 0;
	}
	if (p != 0 && *p == '\0')
		p--;
	return p;
}

/*
 * The replacement for the match from p to m, on the end of out.  (0111)
 */
catsub(s, p, m, o)
	register char *s;
	char *p, *m;
	register char *o;
{
	register char *q;
	register int c;

	while (*o)
		o++;
	for (; *s; s++) {
		c = SC(*s & 0377);
		if (c == -10)
			for (q = p; q < m; q++)
				*o++ = *q;
		else if (c >= -9 && c <= -1) {
			c = -c;
			if (tagstart[c] != 0 && tagend[c] != 0)
				for (q = tagstart[c]; q < tagend[c]; q++)
					*o++ = *q;
		} else
			*o++ = c;
	}
	*o = '\0';
}

/*
 * A line of at most n - 1 characters, newline included.  Whether the file
 * ended is the answer, and it is if the end was met at all.  (13ef)
 */
getl(s, n)
	register char *s;
	register int n;
{
	register int c;

	while (--n > 0) {
		c = getchar();
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

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *p, *lastm, *m, *o;
	register int i;
	char *to;

	if (argc < 2)
		fatal("usage: change from to.");
	if (makpat(argv[1], 0) == (char *)-3)
		fatal("illegal from pattern.");
	to = argc < 3 ? "" : argv[2];
	if (makesub(to, sub, 0) == (char *)-3)
		fatal("illegal to pattern.");
	lineptr = line;
	while (!getl(line, LINESZ)) {
		out[0] = '\0';
		lastm = 0;
		p = line;
		while (*p) {
			for (i = 1; i <= NTAGS; i++)
				tagstart[i] = tagend[i] = 0;
			m = amatch(p, head);
			if (m != 0 && m != lastm) {
				catsub(sub, p, m, out);
				lastm = m;
			}
			if (m == 0 || m == p) {
				for (o = out; *o; o++)
					;
				*o++ = *p++;
				*o = '\0';
			} else
				p = m;
		}
		fputs(out, stdout);
	}
	exit(0);
}
