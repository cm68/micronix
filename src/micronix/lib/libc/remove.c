/*
 * remove is just unlink
 *
 */
#include <unistd.h>
extern int	unlink(char *pathname);

remove(s)
char *	s;
{
	return unlink(s);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
