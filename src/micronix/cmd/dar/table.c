# include "dar.h"


/*
 * table, produce a table of contents of the dos volume.
 * according to the argument list.
 */

table ()
	{
	char *f;
	int i, x;
	struct entry e;

	for (i = 0; i < nfiles; i++)
		{
		f = files [i];

		x = d_stat (f, &e);	/* look up the file */

		if (x < 0)
			{
			nosuch (f);
			}

		else if (x == 0 || (e.att & DIRECTORY))
			{
			tabdir (x, cpystr (path, f, "/", NULL));
			}
		else
			{
			tabfile (f, &e);
			}
		}
	}

tabfile (a, e)
	char *a;
	struct entry *e;
	{
/*
 * Example:      2837 May 24 14:24 1984 bio.c
 */


	if (verbose)
		{
		prsize (e);
		prtime (e);
		}

	prname (a);
 	}

prsize (e)
	struct entry *e;
	{
	long a;

	if (e->att & DIRECTORY)
		{
		a = chainlength (e->start) * d->bpg;
		}
	else
		{
		a = e->highsize;
		a <<= 16;
		a |= e->lowsize;
		}

	printf ("%6lu ", a);
	}

/*
 * perform the table function for a single directory
 *
 * a is the first cluster number of the chain of data blocks
 * b is the accumulated path name so far
 */

tabdir (a, b)
	unsigned a;
	char *b;
	{
	unsigned char c, buf [PATHSIZE];
	char *p;
	int size; 
	struct subdir s;
	struct entry *f;

	newdir (a, &s);

	f = s.buf;
	size = s.size;		/* no. of ents, this directory */

/*
 * scan directory
 */

	for (; size; size--, f++)
		{
		c = *f->name;

		if (c != NOENT && c != 0 && c != '.')
			{
			name (f, b);		/* add new component */

			tabfile (path);

			if (f->att & DIRECTORY)
				{
				p = b;

				while (*p)
					p++;

				*p++ = '/';
				*p = 0;

				tabdir (f->start, p);
				}
			}
		}

	olddir (&s);
	}

prname (a)
	char *a;
	{
	if (*a == '.' && a[1] == '/')
		a += 2;
		
	printf ("%s\n", a);
	}
