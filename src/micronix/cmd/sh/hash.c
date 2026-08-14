/*
 * hash.c - the search path, remembered
 *
 * micronix/cmd/sh/hash.c
 *
 * man1/sh.1, under "Directory Hashing":
 *
 *	The shell internalizes the contents of the directories in the
 *	search path, so that in order to locate a command, it does not
 *	need to refer to the file system.
 *
 * Without it every command costs an access() for each directory
 * ahead of the one it is in - three syscalls to run something in
 * /usr/bin - and under the simulator a syscall is dear.
 *
 * WHAT IS KEPT IS A HASH AND NOT THE NAME, and not to save room -
 * a process has the whole 64k address space to itself and the machine
 * has a megabyte behind it, so the 2k of names for /bin's 121 files
 * and /usr/bin's 17 would be affordable.  It is that the name would
 * buy nothing.  The question being asked is only COULD this directory
 * hold this name, and a hit has to be confirmed with access() either
 * way, because what findcmd wants to know is whether the file is
 * EXECUTABLE and no table of names says that.  So the hash costs one
 * syscall on a collision and the name would have cost the same
 * syscall on every hit.  276 bytes rather than 2k is the smaller
 * reason and comes free.
 *
 * A MISS FALLS BACK TO SEARCHING.  The page describes the original's
 * behaviour when a program is added to the path mid-session - "it may
 * be necessary to enter the command's name twice" - which is the
 * cache being believed when it is stale.  Here a name that no
 * directory claims is looked for anyway, so a new program is found
 * the first time.  That costs the extra access()es only when the
 * command is genuinely absent or genuinely new.
 *
 * "." IS cached, and this is what makes the difference.  Traced side
 * by side, running "ls" costs the image exactly one syscall -
 *
 *	exec("/bin/ls")
 *
 * where an unhashed search costs three: access("./ls") failing,
 * access("/bin/ls") succeeding, then the exec.  The image never looks
 * at "./ls" at all, so it knew the current directory did not hold it
 * without asking - "." is in the tables like the rest.  A hit is
 * therefore not confirmed either: the path is built and exec'd, and
 * exec failing is the only confirmation there is.
 *
 * The cost of caching "." is that it moves with every cd, so the
 * tables are built again there.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/dir.h>
#include "sh.h"

#define DIRSIZ  14

/*
 * One directory of the path: the hashes of the names in it, or a null
 * table for one we do not cache or could not read.
 */
struct hdir {
	UINT *h;
	int n;
};

static struct hdir hdirs[MAXPATHV];
static int hashed;                      /* built at least once */

/*
 * Any cheap spread will do - this is a filter, not a dictionary, and
 * a collision costs one access().  Shift and add over the name.
 */
static UINT
hash(s)
char *s;
{
	UINT h;

	h = 0;
	while (*s)
		h = (h << 3) + (h >> 13) + (*s++ & 0xff);
	return h;
}

static void
freedir(d)
struct hdir *d;
{
	if (d->h)
		free(d->h);
	d->h = (UINT *)0;
	d->n = 0;
}

/*
 * Read one directory into hashes.  Twice over, as glob does: once to
 * count so the room taken is the room needed, once to fill.
 */
static void
readdir(d, name)
struct hdir *d;
char *name;
{
	struct dir e;
	char nm[DIRSIZ + 1];
	FILE *f;
	int count;
	int i;

	freedir(d);
	if (!(f = fopen(name, "r")))
		return;                     /* no cache: it will be searched */

	count = 0;
	while (fread((char *)&e, sizeof(e), 1, f) == 1)
		if (e.ino)
			count++;
	if (count == 0) {
		fclose(f);
		return;
	}
	if (!(d->h = (UINT *)malloc(count * sizeof(UINT)))) {
		fclose(f);
		return;
	}

	rewind(f);
	i = 0;
	while (i < count && fread((char *)&e, sizeof(e), 1, f) == 1) {
		if (!e.ino)
			continue;
		strncpy(nm, e.name, DIRSIZ);
		nm[DIRSIZ] = '\0';
		d->h[i++] = hash(nm);
	}
	fclose(f);
	d->n = i;
}

/*
 * Throw the tables away.
 *
 * This rather than rebuilding on the spot, because the moment a table
 * goes stale is not the moment anyone wants it back: cd three times
 * before running anything and an eager rebuild does the work three
 * times, and a path set to directories that do not exist would have
 * them read for nothing.  Nothing is read until a command is looked
 * for, and until then findcmd falls back to searching, which is what
 * it did before there were tables at all.
 */
void
hashflush()
{
	int i;

	for (i = 0; i < MAXPATHV; i++)
		freedir(&hdirs[i]);
	hashed = 0;
}

/*
 * Build, from whatever pathv says now.  Called on demand rather than
 * by anyone who changes things - see hashflush above.
 */
void
hashpath()
{
	int i;

	for (i = 0; i < MAXPATHV; i++) {
		freedir(&hdirs[i]);
		if (!pathv[i])
			continue;
		readdir(&hdirs[i], pathv[i]);
	}
	hashed = 1;
}

/*
 * Could pathv[i] hold this name?
 *
 * 1 yes or perhaps, 0 certainly not, -1 no cache for it - and the
 * caller must tell the last two apart, because "certainly not" is
 * what saves the syscall and "no cache" is what must not.
 */
int
inhash(i, name)
int i;
char *name;
{
	UINT h;
	int j;

	if (!hashed)
		hashpath();
	if (i < 0 || i >= MAXPATHV || !hdirs[i].h)
		return -1;

	h = hash(name);
	for (j = 0; j < hdirs[i].n; j++)
		if (hdirs[i].h[j] == h)
			return 1;
	return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
