/*
 * mkstemp.c - create unique temporary file
 */

#include <fcntl.h>

extern int getpid(void);

static int counter;
static char hexc[] = "0123456789abcdef";

int
mkstemp(tmpl)
char *tmpl;
{
	char *p;
	int pid, val, fd, i, k;

	/* find end of template */
	for (p = tmpl; *p; p++)
		;

	/* back up over XXXXXX */
	if (p - tmpl < 6)
		return -1;
	p -= 6;

	/* verify template ends with XXXXXX */
	for (i = 0; i < 6; i++)
		if (p[i] != 'X')
			return -1;

	pid = getpid();

	/* try several times to find unique name */
	for (i = 0; i < 100; i++) {
		val = (pid << 8) + counter++;

		/* fill in template with hex digits */
		for (k = 0; k < 6; k++) {
			p[k] = hexc[val & 0xf];
			val >>= 4;
		}	

		fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL);
		if (fd >= 0)
			return fd;
	}
	return -1;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
