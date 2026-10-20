/*
 * getenv - search the environment the kernel put on the stack.
 *
 * crt0 stores the vector's address in environ.  A program started
 * through exec (call 11) has an empty one, so the loop stops at once.
 *
 * The old version read a file called ENVIRON in the current directory
 * and cached it on first call.  That was the CP/M branch of the HiTech
 * C library, where the operating system supplied no environment at
 * exec; it was not an inheritance mechanism, had no writer anywhere in
 * the tree, and changed with the current directory.  The stack vector
 * replaces it.
 */
#include	<string.h>

extern char **	environ;

char *
getenv(s)
char *	s;
{
	register char **	xp;
	short			i;

	i = strlen(s);
	for (xp = environ ; xp && *xp ; xp++)
		if (strncmp(*xp, s, i) == 0 && (*xp)[i] == '=')
			return *xp + i+1;
	return (char *)0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
