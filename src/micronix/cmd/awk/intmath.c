/*
 * Integer replacements for the soft-float runtime's math entry points.
 * awk's numbers are AWKFLOAT (a 32-bit long); the transcendental
 * builtins collapse to integer approximations, and ftoa formats a
 * plain decimal number.
 */
#include "awk.h"

/*
 * compare two awkfloats: -1, 0, +1.  A function, not a macro, because
 * ccc turns the -1 in `(a)>(b)?1:(a)<(b)?-1:0` into 255 when the
 * operands are 32-bit struct fields (relop's fcmp).  (CODEGENGAPS 23.)
 */
AWKFLOAT
fcmp(a, b)
AWKFLOAT a, b;
{
	if (a > b)
		return 1;
	if (a < b)
		return -1;
	return 0;
}

/* floor(sqrt(x)) by Newton's method on the bit level */
AWKFLOAT
isqrt(x)
AWKFLOAT x;
{
	AWKFLOAT r, bit;

	if (x <= 0)
		return 0;
	r = 0;
	for (bit = 1L << 30; bit > x; bit >>= 2)
		;
	while (bit) {
		if (x >= r + bit) {
			x -= r + bit;
			r = (r >> 1) + bit;
		} else
			r >>= 1;
		bit >>= 2;
	}
	return r;
}

/* floor(log2(x)); awk's log is natural, so scale by ln 2 ~ 0.693 */
AWKFLOAT
ilog(x)
AWKFLOAT x;
{
	AWKFLOAT n;

	if (x <= 0)
		return 0;
	n = 0;
	while (x > 1) {
		x >>= 1;
		n++;
	}
	return (n * 693L) >> 10;
}

/* 2^x as a rough inverse of ilog (x is a natural log) */
AWKFLOAT
iexp(x)
AWKFLOAT x;
{
	long n;

	n = (x * 1477L) >> 10;		/* x / ln(2) */
	if (n <= 0)
		return 1;
	if (n >= 30)
		return 1L << 30;
	return 1L << n;
}

char *
ftoa(buf, x, prec)
char *buf;
AWKFLOAT x;
int prec;
{
	sprintf(buf, "%ld", x);
	return buf;
}
