/*
 * libutil.c - Shared utility functions for ccc
 */
#include <stdarg.h>
#include "libutil.h"

/*
 * Minimal formatter - handles %s, %d, %c, %% (no width/padding)
 * Returns pointer to end of written string
 *
 * %x went when nothing in the tree had ever asked for one, and %ld
 * went to fmtlong.c so that it is linked by the one program that
 * wants it.  Between them they were 476 of this function's 1017
 * bytes, in all four passes, and long division for the %ld path that
 * only cpp ever took.
 */
char *
fmtstr(char *buf, char *fmt, ...)
{
	va_list ap;
	char *p = buf;
	char *s;
	unsigned int n;
	int neg, i;

	va_start(ap, fmt);
	while (*fmt) {
		if (*fmt != '%') {
			*p++ = *fmt++;
			continue;
		}
		fmt++;
		switch (*fmt++) {
		case 's':
			s = va_arg(ap, char *);
			while (*s) *p++ = *s++;
			break;
		case 'd':
			i = va_arg(ap, int);
			neg = i < 0;
			n = neg ? -(unsigned int)i : (unsigned int)i;
			s = p;
			do { *p++ = '0' + (n % 10); n /= 10; } while (n);
			if (neg) *p++ = '-';
			/* reverse */
			for (i = 0; i < (int)(p - s) / 2; i++) {
				char t = s[i]; s[i] = p[-1-i]; p[-1-i] = t;
			}
			break;
		case 'c':
			*p++ = (char)va_arg(ap, int);
			break;
		case '%':
			*p++ = '%';
			break;
		}
	}
	va_end(ap);
	*p = 0;
	return p;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
