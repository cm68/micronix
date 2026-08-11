/*
 * copy n bytes from s to d
 *
 */

memcpy(d, s, n)
register char *	d, * s;
register int	n;
{
	while(n--)
		*d++ = *s++;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
