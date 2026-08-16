/*
 * remove is just unlink
 *
 */
extern int	unlink();

remove(s)
char *	s;
{
	return unlink(s);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
