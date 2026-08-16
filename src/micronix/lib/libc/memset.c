/*
 * set memory to a character value
 *
 * memset(p, c, n) - VALUE second, COUNT third, as C has always had
 * it.  This file had them the other way around, so every standard
 * call - memset(p, 0, len) - asked for zero bytes of twelve and set
 * nothing at all.  permalloc's "zeroed" guarantee rested on this,
 * and held only where the freshly loaded image happened to be zero
 * underneath; the intern pool found the places where it was not.
 */

memset(p, c, n)
register char *	p;
int	c;
register int	n;
{
	while (n--)
		*p++ = c;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
