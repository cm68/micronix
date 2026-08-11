/*
 * rename - give a file a new name
 *
 * lib/libc/rename.c
 *
 * Micronix has no rename system call, so it is the old two step: make
 * the new name point at the same file, then take the old name away.
 * That is not atomic, and a crash between the two leaves both names,
 * which is the behaviour every system had before rename became a call
 * of its own.
 *
 * Written because wslib wants it and nothing provided it.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

int
rename(from, to)
char *from;
char *to;
{
	/*
	 * Take the new name away first if it is there.  link refuses to
	 * overwrite, so without this a rename onto an existing file
	 * would simply fail.
	 */
	unlink(to);

	if (link(from, to) < 0)
		return -1;

	if (unlink(from) < 0)
		return -1;

	return 0;
}
