/*
 * fmtlong.c - a long in decimal, on its own
 *
 * This was %ld inside fmtstr, which put the long division that only it
 * needs into every program that formats anything.  cpp is the only one
 * that ever prints a long - a numeric constant it is passing on, and
 * the running value of an enum - and c0, c1 and peep were carrying the
 * conversion and qdiv/qmod behind it for nothing.
 *
 * Its own source, so it is its own member of libccc.a and arrives only
 * where it is called.
 */
#include "libutil.h"

/*
 * Write v into buf in decimal, terminate it, and return a pointer to
 * the null - the same contract fmtstr keeps, so the two can be strung
 * together to build one string.
 */
char *
fmtlong(char *buf, long v)
{
	char *p = buf;
	char *s;
	int neg, i;

	neg = v < 0;
	if (neg)
		v = -v;
	s = p;
	/*
	 * Digits come out backwards and the sign goes on after them, so
	 * the reversal below takes the sign with it and leaves it in
	 * front.
	 */
	do { *p++ = '0' + (int)(v % 10); v /= 10; } while (v);
	if (neg)
		*p++ = '-';
	for (i = 0; i < (int)(p - s) / 2; i++) {
		char t = s[i]; s[i] = p[-1-i]; p[-1-i] = t;
	}
	*p = 0;
	return p;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
