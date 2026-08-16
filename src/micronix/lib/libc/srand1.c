/*
 * initialize random number generator by prompting input
 *
 */
extern int	kbhit();

srand1(s)
char *	s;
{
	int	i;
	while (*s)
		putchar(*s++);
	while (!kbhit())
		i++;
	srand(i);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
