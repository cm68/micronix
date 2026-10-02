/*
 * Whitesmiths libc spellings, restored for the original dar source.
 *
 * The tree dropped these names when it moved to the standard C library,
 * so they are provided here as thin wrappers.  cpystr and putstr are the
 * Whitesmiths NULL-terminated string-list functions; they are fixed to
 * the widest call in dar rather than varargs, which ccc cannot build.
 */
#include <string.h>

extern int write();

char *
cpystr(dst, s1, s2)
char *dst, *s1, *s2;
{
	strcpy(dst, s1);
	if (s2)
		strcat(dst, s2);
	return dst;
}

void
putstr(fd, s1, s2, s3)
int fd;
char *s1, *s2, *s3;
{
	write(fd, s1, strlen(s1));
	if (s2)
		write(fd, s2, strlen(s2));
	if (s3)
		write(fd, s3, strlen(s3));
}

int
itob(buf, n, base)
char *buf;
int n;
int base;
{
	char tmp[16];
	int i, len;
	int d;
	int neg;

	neg = 0;
	if (n < 0) {
		neg = 1;
		n = -n;
	}

	i = 0;
	do {
		d = n % base;
		if (d < 10)
			tmp[i++] = d + '0';
		else
			tmp[i++] = d - 10 + 'a';
		n = n / base;
	} while (n);

	len = 0;
	if (neg) {
		*buf++ = '-';
		len = 1;
	}

	while (i > 0) {
		i--;
		*buf++ = tmp[i];
		len++;
	}

	*buf = 0;
	return len;
}

static void
sort1(lo, hi, cmp, exch, base)
unsigned lo, hi;
int (*cmp)();
int (*exch)();
char **base;
{
	unsigned i, last;

	if (lo >= hi)
		return;

	(*exch)(lo, (lo + hi) / 2, base);
	last = lo;

	for (i = lo + 1; i <= hi; i++)
		if ((*cmp)(i, lo, base) < 0) {
			last++;
			(*exch)(last, i, base);
		}

	(*exch)(lo, last, base);

	sort1(lo, last - 1, cmp, exch, base);
	sort1(last + 1, hi, cmp, exch, base);
}

void
sort(n, cmp, exch, base)
unsigned n;
int (*cmp)();
int (*exch)();
char **base;
{
	if (n > 1)
		sort1(0, n - 1, cmp, exch, base);
}
