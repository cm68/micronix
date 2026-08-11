/*
 * Formatted number printing
 *
 */

#define	NDIG	30		/* max number of digits to be printed */
#define	putch(x)	(*pputch)(x)

_pnum(i, f, w, s, base, pputch)
unsigned long	i;
unsigned char	base;
void	(*pputch)();
unsigned char	s;
char	f, w;
{
	register char *	cp;
	unsigned char	fw;
	char		buf[NDIG];

	if (f > NDIG)
		f = NDIG;

	if (s && (long)i < 0)
		i = -i;
	else
		s = 0;
	if (f == 0 && i == 0)
		f++;

	cp = &buf[NDIG];
	while (i || f > 0) {
		*--cp = "0123456789ABCDEF"[i%base];
		i /= base;
		f--;
	}
	fw = f = (&buf[NDIG] - cp) + s;
	if (fw < w)
		fw = w;
	while (w-- > f)
		putch(' ');
	if (s)
		putch('-');
	else
		++f;
	while (--f)
		putch(*cp++);
	return fw;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
