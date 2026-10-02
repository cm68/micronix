
/*
 * dar - dos archiver
 * 	DOS - Micronix liason
 *
 *	Len Edmondson
 */

# include "dar.h"

	
init (ac, av)
	char **av;
	{
	INTERN *arg, sum;
	
	if (ac == 1)
		inter ();	/* interactive mode */

	if (ac > 1)
		device = av[1];

	if (ac > 2)
		doflag (av[2]);		/* flags */

	if (ac > 3)
		{
		files = av + 3;		/* file list */
		nfiles = ac - 3;
		}

/*
 * The default file list is .  (The current directory)
 */

	if (nfiles <= 0)
		{
		static char *a = ".";

		files = &a;
		nfiles = 1;
		complete [0] = YES;

		if (dflag)
			exit (0);
		}

	sum = dflag + pflag + rflag + tflag + xflag;

	if (sum == 0)
		{
		tflag = YES;		/* default flag */
		verbose = YES;
		}

	else if (sum > 1)
		{
		atmost ();
		}

	arglist ();

	duptest ();

	changename ();
	}


main (ac, av)
	UCOUNT ac;
	TEXT **av;
	{
	init (ac, av);
	dar ();
	finish ();
	exit (YES);
	}

finish ()
	{
	INTERN UCOUNT i;

/*
	if (!rflag)
		{
		for (i = 0; i < nfiles; i++)
			{
			if (complete[i] == NO)
				{
				put (files[i]);
				put (": file not on floppy diskette.\n");
				}
			}
		}
*/

	if (verbose)
		space ();	/* report space status */
	}


doflag (a)
	FAST TEXT *a;
	{
	INTERN sum;

	for (; *a; a++)
		{
		*a = tolower (*a);

		switch (*a)
			{
			case 'c':
				cflag = YES;
				break;
			case 'd':
				dflag = YES;
				break;
			case 'p':
				pflag = YES;
				break;
			case 'r':
				rflag = YES;
				break;
			case 't':
				tflag = YES;
				break;
			case 'x':
				xflag = YES;
				break;
			case 'v':
				verbose = YES;
				break;
			case '-':
				break;
			default:
				usage ();
				break;
			case '?':
				dbflag = YES;
				break;
			}
		}
	}

atmost ()
	{
	put ("At most one of d, r, t, x may be specified.\n");
	exit (NO);
	}

/*
 * print a debugging diagnostic
 */

db (a)
	char *a;
	{
	if (dbflag)
		put (a);
	}

put (a)
	char *a;
	{
	fputs (a, stdout);
	}
