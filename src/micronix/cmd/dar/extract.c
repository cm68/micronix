# include "dar.h"



/*
 * extract each named file and all descendents
 */

extract ()
	{
	char **f;
	unsigned char c;
	int x;
	struct subdir s;
	struct entry *e;
	char dbuf [BUFSIZE], tbuf [BUFSIZE], buf [BUFSIZE];

	for (f = files; f < &files [nfiles]; f++)
		{
		x = d_namei (*f);

		if (x < 0)
			{
			nosuch (*f);
			continue;
			}

		if (d_isdir (*f))
			{
			newdir (x, &s);
			exdir (s.buf, s.size, cpystr (path, *f, "/", NULL));
			olddir (&s);
			}

		else 		/* plain file */
			{
			cpystr (dbuf, parent (*f), NULL);
			cpystr (tbuf, tail (*f), NULL);

			x = d_namei (dbuf);

			if (x < 0)
				{
				nosuch (*f);
				continue;
				}
	
			newdir (x, &s);

			e = d_lookup (tbuf, &s);

			if (e)
				{
				exfile (e, *f);
				}
			else
				{
				nosuch (*f);
				}

			olddir (&s);
			}
		}
	}

/*
 * extract each of the files of a single directory (recursive)
 */

exdir (f, size, p)
	struct entry *f;
	unsigned size;
	char *p;
	{
        char buf [NAMESIZE];
	short n, i;
	unsigned char c;
	char *q;


	for (i = size; i; i--, f++)
		{
		c = *f->name;

		if (c == NOENT || c == 0 || c == '.')
			{
			continue;
			}

		name (f, p);

		if (f->att & DIRECTORY)
			{
			struct subdir s;

			q = p + lenstr (p);

			if (q != path)
				{
				*q++ = '/';
				*q = 0;
				}

			newdir (f->start, &s);
			exdir (s.buf, s.size, q);
			olddir (&s);
			}

		else
			{
			exfile (f, path);
			}
		}
	}

/*
 * extract one file
 */

exfile (a, b)
	struct entry *a;
	char *b;
	{
	char buf [CLUSTERSIZE];
	int out;
	struct entry *f;
	unsigned block, togo;
	unsigned long bleft;


	if (verbose)
		put ("x "), prname (b);

	if (xflag)
		{
		scopy (parent (b), buf);
		mkdir (buf);

		out = create (b, WRITE, BINARY);

		if (out < 0)
			{
			perror (b);
			return;
			}
		}

/*
 * bleft is the number of bytes left to copy.
 * block is the current cluster number to read.
 */

	bleft = entsize (a);		/* measure the file */
	block = a->start;
	while (bleft && block)
		{

		if (bleft < d->bpg)
			togo = bleft;		/* partial */
		else
			togo = d->bpg;		/* full */

		gio (block, buf, READ); /* read the group */

		if (pflag)
			{
			if (write (STDOUT, buf, togo) < 0)
				{
				perror (b);
				return NO;
				}
			}

		if (xflag)
			{
			if (write (out, buf, togo) < 0)
				{
				perror (b);
				return NO;
				}
			}

		bleft -= togo;
		block = nextblock (block);
		}

	if (xflag)
		{
		close (out);
		}
	}
