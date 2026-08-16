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
	unsigned char	sc;		/* the sign to print, or none */
	char		buf[NDIG];

	if (f > NDIG)
		f = NDIG;

	/*
	 * s says what sign treatment the conversion asked for: 0 none,
	 * 1 signed, 2 signed and print a "+" when it is not negative -
	 * which is what "%+d" wants.
	 */
	sc = 0;
	if (s) {
		if ((long)i < 0) {
			i = -i;
			sc = '-';
		} else if (s == 2)
			sc = '+';
	}
	if (f == 0 && i == 0)
		f++;

	cp = &buf[NDIG];
	while (i || f > 0) {
		/*
		 * Lower case, which is what C says %x is.  This libc has
		 * no upper case hex to ask for - see doprnt, where an
		 * upper case conversion means long - so nothing loses a
		 * spelling it could have had, and %x now agrees with
		 * every other printf.
		 */
		*--cp = "0123456789abcdef"[i%base];
		i /= base;
		f--;
	}
	fw = f = (&buf[NDIG] - cp) + (sc ? 1 : 0);
	if (fw < w)
		fw = w;
	while (w-- > f)
		putch(' ');
	if (sc)
		putch(sc);
	else
		++f;
	while (--f)
		putch(*cp++);
	return fw;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
