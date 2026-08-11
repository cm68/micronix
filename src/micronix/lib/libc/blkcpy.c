/*
 * copy sp to dp for n bytes
 *
 */
blkcpy(dp, sp, n)
	register char *dp, *sp;
	register unsigned n;
{
	while (n--)
		*dp++ = *sp++;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
