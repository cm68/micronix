# include "dar.h"


/*
 * mkdir - make new unix directory
 * 	   see that all the path components exist
 */

mkdir (new)
	char *new;
	{
	char buf [PATHSIZE], *p;

	if (!*new || equal (new, "/") || equal (new, "."))
		return;

/*
 * make your parent first, then make yourself
 */

	scopy (parent (new), buf);

	mkdir (buf);

	if (fexists (new))	/* all done */
		{
		return;
		}


	if (mknod (new, DIRMODE, 0) < 0)	/* create directory */
		{
		perror (new);
		return;
		}

	cpystr (buf, new, "/.", NULL);

	if (link (new, buf) < 0)
		{
		unlink (new);		/* back out */

		perror (new);
		return;
		}

	cpystr (buf, new, "/..", NULL);

	if (link (parent (new), buf) < 0)
		{
		unlink (buf);		/* back out */
		unlink (new);

		perror (new);
		return;
		}
	}
