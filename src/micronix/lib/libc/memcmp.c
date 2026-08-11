/*
 * compare memory
 *
 */

memcmp(s1, s2, n)
register char *	s1, * s2;
register int	n;
{
	short	i;

	while(n--)
		if(i = *s1++ - *s2++)
			return i;
	return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
