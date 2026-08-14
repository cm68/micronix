/*
 * man - print the manual page for a command
 *
 * cmd/man/man.c
 *
 * THIS IS A RECONSTRUCTION.  There is no surviving source for /bin/man;
 * this file was written from the disassembly of the /bin/man binary on the
 * Micronix 1.6 filesystem - see man.dist, man.dis, man.ctl and README
 * beside this file.  Every function below corresponds to one function in
 * that binary, in the same order, and the odd bits are reproduced rather
 * than tidied away, because the odd bits are what the callers were
 * written against.
 *
 * The original was compiled by Whitesmith's C, and the shape of the object
 * is the same as /etc/init's:
 *
 *  - three register variables in fixed cells, saved by c.ent2.  Functions
 *    that use them are marked 'register' here - only equal() does.
 *  - what look like locals are often in static storage, laid out in source
 *    order.  Those are written as file-scope statics below, which is what
 *    they are.  show() keeps five of them.
 *  - buffer sizes come from the stack frame sizes, so they are exact even
 *    where they look silly: exists() really does put a 512 byte buffer on
 *    the stack to hold a 36 byte struct stat, the same way init's does.
 *  - there is no bss.  nm -v says text 8662 at 0x0100, data 525 at 0x3000,
 *    bss 0 - so the zeroed globals are in the data segment.
 *
 * What it does: work along argv.  An argument of all digits is the section
 * to look in; "-" turns off the -t that would otherwise be passed to form;
 * anything else is a page to show.  For each page, find the file - either
 * in the section given, or by trying sections 0 through 9 in turn, or under
 * /usr/help if we were invoked as "help" - and hand it to form(1).  The
 * search is a raw read of the directory, comparing each entry with its
 * suffix cut off, which is how "man ls" finds "ls.1".
 *
 * man and help are the same program.  main() looks for "help" inside
 * argv[0], so it works through either name and through any path.
 */
#include <types.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/sgtty.h>
#include <sys/dir.h>

/*
 * A directory is read raw, sixteen bytes at a time, so this is the v6
 * entry: a two byte inumber and fourteen name bytes with no terminator.
 *
 * d_term is the seventeenth byte, and is not on disk.  The object keeps a
 * zero there and reads only sixteen, which is what makes a name filling
 * all fourteen bytes still end - so the read below is DENTSZ and not
 * sizeof, and the two differ on purpose.
 *
 * <sys/dir.h> has struct dir for the same thing, but only the sixteen
 * bytes of it, and the terminator has to be inside one object rather than
 * a second static that happens to follow.
 */
#define DIRSZ	14			/* name bytes on disk */
#define DENTSZ	16			/* and the whole entry */

struct dent {
	int d_ino;
	char d_name[DIRSZ];
	char d_term;			/* not on disk - see above */
};

static struct dent dent;			/* 0x301e, d_term at 0x302e */

static int istty;				/* 0x3000 */
static int helpmode;				/* 0x3002 */
static char *section;				/* 0x3004 */
static char *argvec[8];				/* 0x3006, form's argv */
static char **av;				/* 0x3016, walks argvec */
static char *scan;				/* 0x3018 */
static char *found;				/* 0x301a */

/*
 * The digit that names the section being tried.
 *
 * In the binary this is a char * in the data segment pointing at the
 * string "0" in the TEXT segment, and the search increments the character
 * through it in place - Whitesmith's puts literals in text and Micronix
 * does not write protect it, so "0"++ is a thing that worked.  A two byte
 * buffer says the same without needing that to be true.  show() puts the
 * '0' back each time round, which the literal got for free from being
 * reloaded with the program - and would NOT have got on a second call, so
 * the original only ever worked because show() sets it before the loop
 * too.
 */
static char sectbuf[2];
static char *sectp;

extern char *concat();

/*
 * The section argument is a run of digits.  An empty string is a section
 * too - the loop finds the NUL first and says yes - which is what lets
 * "man '' ls" put the pages of section "" in play.  Nothing does that.
 *
 * The lettered subsections the printed manual uses - 3s, 3m, 3c - are not
 * accepted here, so "man 3s printf" treats 3s as a page name and fails to
 * find it.  That is the binary's behaviour and not an omission in this
 * file; see README.
 */
issection(s)
char *s;
{
	while (*s) {
		if (*s < '0' || *s > '9')
			return 0;
		s++;
	}
	return 1;
}

/*
 * Does the first string occur inside the second?
 *
 * Used once, on argv[0], so that man knows it was run as help whatever
 * path it was found under.  The outer loop walks the haystack and the
 * inner one matches from there; a needle that runs out is a hit.
 */
contains(needle, hay)
char *needle;
char *hay;
{
	char *p;
	char *q;

	while (*hay) {
		p = needle;
		q = hay;
		while (*p && *p == *q) {
			p++;
			q++;
		}
		if (*p == 0)
			return 1;
		hay++;
	}
	return 0;
}

/*
 * Does the file exist?
 *
 * The buffer is 512 bytes for a structure that needs 36.  init's exists()
 * does the same thing and this is the same code; the frame size is what
 * the disassembly gives, so it is reproduced rather than trimmed.
 */
exists(path)
char *path;
{
	char buf[512];

	return stat(path, buf) >= 0;
}

/*
 * "No documentation for x".  Goes to descriptor 2, unbuffered, one write
 * per string.
 */
nodoc(name)
char *name;
{
	putstr(2, "No documentation for ", name, "\n", 0);
}

usage(a0)
char *a0;
{
	putstr(2, "usage: ", a0, " [section] program ...", 0);
	exit(0);
}

/*
 * Search one directory for a page called name.
 *
 * The directory is opened as an ordinary file and read sixteen bytes at a
 * time, which is what a v6 directory is.  Each entry has its suffix cut
 * off before the comparison - the first '.' and everything after it goes -
 * so "ls" matches the entry "ls.1", and what is returned is the entry
 * name WITH the suffix, because the caller has to build a path from it.
 *
 * The return points into dent, so it is only good until the next call.
 * show() uses it immediately and does not keep it.
 */
char *
find(name, dir)
char *name;
char *dir;
{
	FILE *fp;
	char stem[DIRSZ + 1];
	char *p;

	if ((fp = fopen(dir, "read")) == 0)
		return 0;

	for (;;) {
		fread(&dent, DENTSZ, 1, fp);
		/*
		 * was fp->_flag & 0x30, the old whitesmith flag bits for
		 * end of file and error; feof and ferror ask the same
		 * question and exist in both stdio.h's
		 */
		if (feof(fp) || ferror(fp))
			break;
		dent.d_term = 0;
		if (dent.d_ino == 0)
			continue;		/* a free slot */
		if (equal(dent.d_name, ".") || equal(dent.d_name, ".."))
			continue;

		concat(stem, dent.d_name, 0);
		for (p = stem; *p; p++) {
			if (*p == '.') {
				*p = 0;
				break;
			}
		}
		if (equal(name, stem)) {
			fclose(fp);
			return dent.d_name;
		}
	}
	fclose(fp);
	return 0;
}

/*
 * Run form on the page.
 *
 * The child tries /bin/ first and then /usr/bin, and the second of those
 * cannot work: the string in the binary is "/usr/bin" with no trailing
 * slash, so the path built is "/usr/binform".  It is kept because it is
 * what the binary does and because it costs nothing - /bin/form exists,
 * the first exec succeeds, and the bug has never been reached.  See
 * README.
 *
 * The parent waits for every child rather than for this one; man has no
 * others, so it comes to the same thing.
 */
run(av)
char **av;
{
	int pid;
	int status;
	char path[520];

	if ((pid = fork()) == -1) {
		perror(0);
		return;
	}
	if (pid != 0) {
		while (wait(&status) >= 0)
			;
		return;
	}

	concat(path, "/bin/", av[0], 0);
	execv(path, av);
	concat(path, "/usr/bin", av[0], 0);
	execv(path, av);
	perror(av[0]);
	exit(0);
}

/*
 * Find one page and show it.
 *
 * The name has its own suffix cut off first, so "man ls.1" and "man ls"
 * ask the same question.
 *
 * An empty name, or one beginning with a '.' and so left with nothing,
 * gets the "No documentation" message AND THEN CARRIES ON to look for it
 * anyway - the object calls nodoc and falls through rather than returning.
 * Reproduced: it is harmless, since the search then fails and says so a
 * second time, and it is what the code does.
 *
 * Then, in order: the section if one was given; /usr/help if we are help;
 * otherwise sections 0 through 9 until one answers.
 */
show(name)
char *name;
{
	char dir[512];
	char path[512];
	char spare[512];

	found = 0;
	if (name == 0)
		return;
	if (name[0] == 0)
		nodoc(name);
	if (name[0] == '.')
		nodoc(name);

	for (scan = name; *scan; scan++) {
		if (*scan == '.') {
			*scan = 0;
			break;
		}
	}

	if (section) {
		concat(dir, "/usr/man/man", section, "/", 0);
		if ((found = find(name, dir)) == 0) {
			nodoc(name);
			return;
		}
		concat(path, dir, found, 0);
	} else {
		found = 0;
		if (helpmode) {
			concat(spare, "/usr/help/", 0);
			found = find(name, spare);
		}
		for (sectp = sectbuf, *sectp = '0';
		     *sectp <= '9' && found == 0;
		     (*sectp)++) {
			concat(spare, "/usr/man/man", sectp, "/", 0);
			found = find(name, spare);
		}
		if (found == 0) {
			nodoc(name);
			return;
		}
		concat(path, spare, found, 0);
	}

	if (!exists(path)) {
		nodoc(name);
		return;
	}

	av = argvec;
	*av++ = "form";
	if (istty)
		*av++ = "-t";
	*av++ = path;
	*av++ = 0;
	run(argvec);
}

/*
 * Walk argv.  A section is remembered, and only the first one is - a
 * second run of digits is taken as a page name, which is why "man 1 2"
 * looks for a page called 2 in section 1.
 *
 * "-" is the only option and it is not spelled like one: it clears the
 * flag that would otherwise pass -t to form.  Nothing was shown at all
 * means the usage, which is how "man" on its own answers.
 */
doargs(argc, argv)
int argc;
char **argv;
{
	int nshown;
	int i;
	char *arg;

	nshown = 0;
	for (i = 1; i < argc; i++) {
		arg = argv[i];
		if (issection(arg) && section == 0) {
			section = arg;
			continue;
		}
		if (equal(arg, "-")) {
			istty = 0;
			continue;
		}
		nshown++;
		show(arg);
	}
	if (nshown == 0)
		usage(argv[0]);
}

/*
 * The -t that form is given when the output is a terminal is decided
 * here, by asking descriptor 1 for its modes: gtty fails on anything that
 * is not a typewriter, and that is the whole test.
 *
 * The exit status is 1 whatever happened, and 0 from the usage - which is
 * the wrong way round and is what the binary does.  Nothing in the system
 * looks at it.
 */
main(argc, argv)
int argc;
char **argv;
{
	struct sgtty sg;

	if (contains("help", argv[0]))
		helpmode = 1;
	istty = gtty(1, &sg) >= 0;
	doargs(argc, argv);
	exit(1);
}

/*
 * ---------------------------------------------------------------------
 * The Whitesmith's library routines man calls that this tree's libc does
 * not carry.
 *
 * These are not man's own functions.  In the binary they sit in the
 * library region, among fopen, perror and execv:
 *
 *	equal	0x1647	   concat   0x1692	putstr	0x16f7
 *
 * They are the same three /etc/init needed, transcribed the same way -
 * cmd/init/init.c has them at 0x30dc, 0x302b and 0x3422 in that binary.
 * Two copies is one too many and libc is where they belong; when it grows
 * them, both go, and the addresses say what to check against.
 * ---------------------------------------------------------------------
 */

/*
 * String equality, 1 or 0.  Compares before testing for the end, so two
 * empty strings are equal and a prefix is not.  This one really does use
 * the register variables - r3 and r1 - which is why it is marked.
 */
equal(a, b)
register char *a;
register char *b;
{
	while (*a == *b) {
		if (*a == 0)
			return 1;
		a++;
		b++;
	}
	return 0;
}

/*
 * Concatenate the strings after dst onto dst, stopping at a null pointer,
 * and return a pointer to the terminating NUL rather than to the start.
 * Nothing here uses the return.
 */
char *
concat(dst, va_alist)
char *dst;
{
	va_list ap;
	char *s;
	char *d;

	d = dst;
	va_start(ap, dst);
	while ((s = va_arg(ap, char *)) != 0) {
		while (*s)
			*d++ = *s++;
	}
	va_end(ap);
	*d = 0;
	return d;
}

/*
 * Write each string to fd, stopping at a null pointer.  One write per
 * string, its length measured first - the object calls strlen and then
 * write, and does not buffer.  That is why the messages come out in the
 * right order against form's output.
 */
putstr(fd, va_alist)
{
	va_list ap;
	char *s;

	va_start(ap, fd);
	while ((s = va_arg(ap, char *)) != 0)
		write(fd, s, strlen(s));
	va_end(ap);
}

/*
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
