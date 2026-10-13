/*
 * macro - macro expansion
 *
 * cmd/macro/macro.c
 *
 * Copies the standard input to the standard output with macros
 * expanded.  A line "define(token,string)" defines token to expand to
 * string; a token is a run of alphanumerics, and its expansion is
 * applied recursively.  In a definition, $n (n a digit) is the nth
 * argument of an invocation "token(a,b,...)", $1 the first; an $n past
 * the last argument expands to nothing.
 *
 * The whole input is read in first, so a source much over INBUFSZ is
 * truncated.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>

#define	alnum(c) (((c)>='A'&&(c)<='Z')||((c)>='a'&&(c)<='z')||((c)>='0'&&(c)<='9'))
#define	digit(c) ((c) >= '0' && (c) <= '9')

#define	MAXMAC	16
#define	NAMLEN	32
#define	DEFLEN	256
#define	MAXARGS	10
#define	MAXDEPTH 16
#define	INBUFSZ	8192

struct macro {
	char	name[NAMLEN];
	char	def[DEFLEN];
	int	ndef;
};

static struct macro macros[MAXMAC];
static int	nmac;

static char	inbuf[INBUFSZ];
static char	*input;
static int	remaining;

/* argument pointers per nesting level: one invocation's args must
   survive the recursion that expands its body */
static char	*arglist[MAXDEPTH][MAXARGS];
static char	**args;
static int	nargs;
static int	depth;

static char	tokbuf[NAMLEN];

static int lookup(char *name);
static void process(void);
static void define_macro(void);
static void read_args(void);

static int
lookup(name)
	char *name;
{
	int i;

	for (i = 0; i < nmac; i++)
		if (strcmp(macros[i].name, name) == 0)
			return i;
	return -1;
}

/*
 * Expand from input/remaining to the standard output, until the
 * current input is used up.  input points into inbuf (stdin) or into
 * a macro's def; the cursor and the argument list are saved and put
 * back around each recursion.
 */
static void
process()
{
	int m, k;

	if (++depth > MAXDEPTH) {	/* runaway recursion */
		depth--;
		return;
	}
	while (remaining > 0) {
		if (alnum(*input)) {
			k = 0;
			while (remaining > 0 && alnum(*input) &&
			       k < NAMLEN - 1) {
				tokbuf[k++] = *input++;
				remaining--;
			}
			tokbuf[k] = '\0';
			if (strcmp(tokbuf, "define") == 0) {
				define_macro();
				continue;
			}
			m = lookup(tokbuf);
			if (m >= 0) {
				char *sav_in;
				int sav_rem;
				char **sav_args = args;
				int sav_nargs = nargs;
				if (remaining > 0 && *input == '(') {
					input++;
					remaining--;
					read_args();
				} else {
					args = (char **)0;
					nargs = 0;
				}
				/* resume after the invocation, not before it */
				sav_in = input;
				sav_rem = remaining;
				input = macros[m].def;
				remaining = macros[m].ndef;
				process();
				input = sav_in;
				remaining = sav_rem;
				args = sav_args;
				nargs = sav_nargs;
			} else
				fputs(tokbuf, stdout);
		} else if (*input == '$' && remaining > 1 && digit(input[1])) {
			int a = input[1] - '0';
			input += 2;
			remaining -= 2;
			if (a >= 1 && a <= nargs) {
				char *sav_in = input;
				int sav_rem = remaining;
				char **sav_args = args;
				int sav_nargs = nargs;
				input = args[a - 1];
				remaining = strlen(args[a - 1]);
				args = (char **)0;
				nargs = 0;
				process();
				input = sav_in;
				remaining = sav_rem;
				args = sav_args;
				nargs = sav_nargs;
			}
		} else {
			putchar(*input);
			input++;
			remaining--;
		}
	}
	depth--;
}

/*
 * A define has been read; input points just past "define".  Take the
 * (name,string) that follows and store it; a define produces no
 * output.  The shape is define(name,string) with no spaces assumed.
 */
static void
define_macro()
{
	char *d;
	int nd, k;

	while (remaining > 0 && (*input == ' ' || *input == '\t')) {
		input++;
		remaining--;
	}
	if (remaining == 0 || *input != '(')
		return;
	input++;
	remaining--;

	k = 0;
	while (remaining > 0 && alnum(*input) && k < NAMLEN - 1) {
		macros[nmac].name[k++] = *input++;
		remaining--;
	}
	macros[nmac].name[k] = '\0';

	while (remaining > 0 && *input != ',') {
		input++;
		remaining--;
	}
	if (remaining == 0)
		return;
	input++;
	remaining--;

	d = macros[nmac].def;
	nd = 0;
	while (remaining > 0 && *input != ')') {
		if (nd < DEFLEN - 1)
			d[nd++] = *input;
		input++;
		remaining--;
	}
	d[nd] = '\0';
	macros[nmac].ndef = nd;
	if (remaining > 0 && *input == ')') {
		input++;
		remaining--;
	}
	nmac++;
}

/*
 * input points just past the '(' of an invocation.  Split what is
 * left up to the matching ')' into comma-separated arguments, stored
 * in this depth's arg slot.  The ',' and ')' are overwritten with
 * '\0' in place.
 */
static void
read_args()
{
	char *start, *e;
	int d;

	args = arglist[depth];
	nargs = 0;
	for (;;) {
		/* whitespace after a comma is the separator, not the arg */
		while (remaining > 0 && (*input == ' ' || *input == '\t')) {
			input++;
			remaining--;
		}
		start = input;
		while (remaining > 0 && *input != ',' && *input != ')') {
			input++;
			remaining--;
		}
		/* trim trailing whitespace and end the arg */
		e = input;
		d = remaining > 0 ? *input : ')';
		while (e > start && (e[-1] == ' ' || e[-1] == '\t'))
			e--;
		*e = '\0';
		if (nargs < MAXARGS)
			args[nargs++] = start;
		if (remaining == 0)
			return;
		input++;
		remaining--;
		if (d == ')')
			return;
	}
}

main()
{
	remaining = fread(inbuf, 1, sizeof inbuf, stdin);
	input = inbuf;
	args = (char **)0;
	nargs = 0;
	depth = 0;
	process();
	exit(0);
}
