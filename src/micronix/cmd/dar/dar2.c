# include "dar.h"

dar ()
	{
	struct stat s;

/*
 * make sure the device is ok
 */

	if (!okdev ())
		{
		usage ();
		}

/*
 * make sure the device or its alternate are not mounted
 */

	mcheck (device, &s);

	devopen ();

	findsize ();

	readdir ();

	if ((dflag || rflag) && !isdos ())
		{
		put ("Not a DOS diskette!\n");
		exit (NO);
		}

	if (dflag)
		delete ();
	else if (rflag)
		replace ();
	else if (tflag)
		table ();
	else if (xflag || pflag)
		extract ();

/*
 * write the DOS directory back out again if need be
 */

	if (ddirty)
		writedir ();
	}


/* 
 * free a list of pointers
 */

freelist (a, b) 
	char **a;
	int b;
	{
	int i;

	for (i = 0; i < b; i++)
		free (a[i]);
	}

dispremote ()
	{
	int old;

	tflag = YES;
	old = nfiles, nfiles = 0;	/* save and restore nfiles */

	dar ();

	nfiles = old;
	tflag = NO;
	}

/*
 * print a list in columar form
 */

compare (a, b, c)
	unsigned a, b;
	char ***c;
	{
	return strcmp ((*c)[a], (*c)[b]);
	}

exchange (a, b, c)
	unsigned a, b;
	char ***c;
	{
	static char *t;

	t = (*c)[a], (*c)[a] = (*c)[b], (*c)[b] = t;
	}

prlist (u, nitem)
	FAST TEXT **u;
	{
	COUNT l;
	UCOUNT n, nrow, r, c, i;

	if (!nitem)
		return;

	sort (nitem, compare, exchange, u);

	nrow = (nitem - 1) / 5 + 1;

	for (r = 0; r < nrow; r++)	/* for each row */
		{

		i = r;			/* i is the item number */

		for (c = 0; c < 5; c++, i += nrow) /* 5 cols. */
			{

			if (i >= nitem)	/* no more this line */
				break;

			put (u[i]);

			if (c != 4)		/* not last col. */
				for (l = 16 - lenstr (u[i]); l > 0; l--)
					put (" ");
			}

		put ("\n");
		}
	}
