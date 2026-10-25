/*
 * env - print the inherited environment
 *
 * cmd/env/env.c
 *
 * Nothing but the vector crt0 found on the stack: the strings are
 * written out one per line, in the order the kernel gave them.  There
 * are no options and no arguments, because the only question this
 * answers is "what did I inherit?" - a program started with exec has
 * an empty environment and prints nothing, and execve is what puts
 * anything here.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <unistd.h>
#include <string.h>

extern char **	environ;

main()
{
	char **	ep;

	for (ep = environ; ep && *ep; ep++) {
		write(1, *ep, strlen(*ep));
		write(1, "\n", 1);
	}
	exit(0);
}
