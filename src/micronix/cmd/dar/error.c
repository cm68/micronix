
/*
 * Having to do with error conditions.
 */

# include "dar.h"

nosuch (a)
	char *a;
	{
	put (a);
	put (": No such file or directory\n");
	}

usage ()
	{
	put ("usage: dar device {cdprtx} files ...\n");
	put ("Example: dar mfa xv\n");
	exit (NO);
	}
