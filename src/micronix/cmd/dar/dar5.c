# include "dar.h"

/*
 * interactive mode
 */

inter ()
	{
	INTERN TEXT devbuf [128], filebuf [128], *f;
	TEXT buf [BUFSIZE];
	struct stat s;


/*	if (!standard ())	/* check for standard environment */
		usage ();

	device = devbuf;
	getdevname ();

	for (;;)
		{
		f = filebuf;		/* the default file list is */
		nfiles = 1;		/* a single file */
		files = &f;

/* main menu */

put ("\n\
\n\
	A) Copy from the DOS diskette\n\
	B) Copy to the DOS diskette\n\
	C) Display file on the DOS diskette\n\
	D) Quit\n\
\n\
Selection: "
);
	

		gets (buf);		/* read response */

		if (feof (stdin))
			exit (YES);

		*buf = tolower (*buf);

		verbose = YES;

		tflag = dflag = rflag = xflag = pflag = NO;

		switch (*buf)
			{
			case 'a':	/* extract */
				{
				int i;
				/* print list of files to choose from */

				dispremote ();

			put ("\nFile to copy (or RETURN for all files): ");

				gets (filebuf);

				if (feof (stdin))
					exit (YES);

				if (!*filebuf)
					nfiles = 0;	/* all files */

				xflag = YES;

				cleanslate ();
				dar ();
				finish ();
				break;
				}

			case 'b':	/* copy to */
				{
				char *names [BUFSIZE];
				int k;

				k = readlocal (names);	/* read in CP/M dir */
				
				prlist (names, k); /* display file names */

			put ("\nFile to copy (or RETURN for all files): ");

				gets (filebuf);

				if (feof (stdin))
					exit (YES);

				if (*filebuf)
					k = trimlist (filebuf, names, k);

				rflag = YES;

				files = names;		/* use our list */
				nfiles = k;

				dar ();
				finish ();
				freelist (names, k);
				break;
				}

			case 'c':	/* delete file(S) */
				{
				int i;

				dispremote ();

				put ("\nFile to delete: ");

				gets (filebuf);

				if (feof (stdin))
					exit (YES);

				if (*filebuf);
					{
					dflag = YES;

					cleanslate ();

					dar ();
					finish ();
					}

				break;
				}

			case 'd':	/* Quit */
				{
				exit (NO);
				}
			}
		}
	}

trimlist (a, b, c)
	char *a, **b;
	int c;
	{
	int i;

	for (i = 0; i < c; i++)
		if (!match (b [i], a))
			{
			free (b[i]);
			b [i--] = b[--c];
			}

	return c;
	}

/*
 * clear out the completion status table
 */

cleanslate ()
	{
	static i;

	for (i = 0; i < nfiles; i++)
		complete [i] = NO;
	}
