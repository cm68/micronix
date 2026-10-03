/*
 * rename - give a file a new name
 *
 * There is no rename system call on this system.  It arrived in 4.2BSD
 * and Micronix is a v6, so the operation has to be built out of the
 * two calls that do exist: make a second name for the file, then take
 * the first one away.
 *
 * THE API IS THE INVARIANT, not this.  Micronix may grow a rename
 * system call, and when it does this file becomes an .s stub in the
 * shape of link.s and nothing above it changes: rename(from, to),
 * zero or -1 with errno set.  So it is worth nothing to write callers
 * around what is underneath it today, and worth something to keep the
 * signature exactly what a caller would expect.
 *
 * link fails if the new name already exists, so the old one goes
 * first.  That is the one place this differs from the real thing: a
 * genuine rename replaces the destination atomically, and this has a
 * window between the unlink and the link where neither name is there.
 * Deliberate.  Nothing on a single-user machine is racing us for it,
 * and the alternative - link to a temporary, unlink, link, unlink -
 * trades the window for a temporary name that a crash would leave
 * behind.  A system call would close it for free, which is another
 * reason not to pay for it here.
 *
 * libcpm has its own, because CP/M has a BDOS call for it.  The
 * driver carries a private copy of this as moveover() in ccc.c: it has
 * to compile on a host as well, where rename is the C library's and
 * means something subtly different.
 */
rename(from, to)
char *from, *to;
{
	unlink(to);
	if (link(from, to) != 0)
		return -1;
	return unlink(from);
}
