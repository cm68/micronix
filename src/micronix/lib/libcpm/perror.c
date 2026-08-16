/*
 * perror - report errno on the standard error
 *
 * This writes on the file descriptor rather than through stdio, the
 * way libu's perror.s does on the other side.  Going through putc
 * and stderr drags the whole stdio machinery - _iob, the buffers,
 * fclose and fflush - into any program that so much as mentions
 * perror.  cpp goes out of its way to avoid exactly that, defining
 * an empty _cleanup so that exit() does not pull it in either, and
 * a perror built on putc undid that and collided with the _cleanup
 * in libc's cleanup.c, which sits in the same object as the _iob
 * table it was really being pulled in for.
 *
 * vim: set tabstop=4 shiftwidth=4 noexpandtab:
 */

extern	int	errno;
extern	char *	sys_err[];
extern	int	sys_ner;

static
ps(s)
char *	s;
{
	char *	p;

	p = s;
	while (*p)
		p++;
	if (p != s)
		write(2, s, p - s);
}

perror(s)
char *	s;
{
	ps(s);
	write(2, ": ", 2);
	if (errno >= 0 && errno < sys_ner)
		ps(sys_err[errno]);
	else
		ps("Unknown error");
	write(2, "\n", 1);
}
