/*
 * initialize random number generator by prompting input
 *
 */
#include <stdio.h>
extern int	kbhit(void);


/* prototypes for functions defined below */
int srand(unsigned);
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
