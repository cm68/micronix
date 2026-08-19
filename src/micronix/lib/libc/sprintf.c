/*
 * formatted output to string
 *
 */
extern int	_doprnt();

static char *	sptr;

static
emit_str(c)
int	c;
{
	*sptr++ = c;
}

sprintf(wh, f, a)
char *	wh;
char *	f;
int	a;
{
	sptr = wh;
	_doprnt(emit_str, f, &a);
	*sptr = 0;
	return sptr - wh;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
