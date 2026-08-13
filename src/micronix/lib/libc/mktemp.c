/*
 * mktemp.c - fill in a unique temporary file name
 *
 * The older half of the pair beside mkstemp.c, and the one ar wants:
 * it names a file without creating it, and hands the template back so
 * the caller can create it however it likes.  ar makes three of these
 * and opens them itself.
 *
 * Same naming scheme as mkstemp - the pid and a counter, in hex, over
 * the six X's - so the two cannot collide with each other either.
 * Where mkstemp settles the race by asking open for O_EXCL, this one
 * has nothing to open, so it asks access whether the name is taken
 * and moves on if it is.  That is a race and always has been; it is
 * why mkstemp exists.
 */

extern int getpid();

static int counter;
static char hexc[] = "0123456789abcdef";

char *
mktemp(tmpl)
char *tmpl;
{
	char *p;
	int pid, val, i, k;

	/* find end of template */
	for (p = tmpl; *p; p++)
		;

	/* back up over XXXXXX */
	if (p - tmpl < 6)
		return tmpl;
	p -= 6;

	/* verify template ends with XXXXXX */
	for (i = 0; i < 6; i++)
		if (p[i] != 'X')
			return tmpl;

	pid = getpid();

	/* try several times to find an unused name */
	for (i = 0; i < 100; i++) {
		val = (pid << 8) + counter++;

		/* fill in template with hex digits */
		for (k = 0; k < 6; k++) {
			p[k] = hexc[val & 0xf];
			val >>= 4;
		}

		if (access(tmpl, 0) != 0)
			return tmpl;
	}

	/*
	 * Out of names.  An empty string is what the callers can
	 * actually notice - ar checks the name it got before using it -
	 * where handing back a template still full of X's would look
	 * like it had worked.
	 */
	*tmpl = '\0';
	return tmpl;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
