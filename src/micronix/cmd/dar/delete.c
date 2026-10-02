# include "dar.h"

/*
 * delete each named file 
 */

delete ()
	{
	char *f;
	int i, x;

	for (i = 0; i < nfiles; i++)
		{
		f = files [i];

		x = d_namei (f);

		if (x < 0)
			{
			nosuch (f);
			continue;
			}

		if (d_isdir (f))
			{
			deldir (x, cpystr (path, f, "/", NULL));
			}

		if (x)
			{
			delfile (f); /* can't delete the root dir. */
			}
		}
	}

/*
 * deldir - delete all the files in a directory hierarchy.
 */

deldir (a, b)
	int a;
	char *b;
	{
	unsigned char c, buf [NAMESIZE];
	char *p;
	int n;
	struct entry *f;
	struct subdir s;

	newdir (a, &s);

	n = s.size;
	f = s.buf;

	for (; n; n--, f++)
		{
		c = *f->name;

/*
 * Unused directory entry slot.
 */

		if (c == NOENT || c == 0)
			continue;

/*
 * Remove directory structural parts silently.
 */

		if (c == '.')
			{
			*f->name = NOENT;
			s.flag |= DIRTY;
			continue;
			}

		name (f, b);

		if (verbose && dflag)
			{
			put ("d ");
			prname (path);
			}

		if (f->att & DIRECTORY)
			{
			p = b;

			while (*p)
				p++;

			*p++ = '/';
			*p = 0;

			deldir (f->start, p);
			}
					
		*f->name = NOENT;
		freechain (f->start);
		s.flag |= DIRTY;
		}

	olddir (&s);
	}

/*
 * delete a single file
 */

delfile (a)
	char *a;
	{
	char buf [PATHSIZE];
	int x;
	struct entry *f;
	struct subdir s;

	strcpy (buf, parent (a));

	x = d_namei (buf);

	if (x < 0)
		{
		nosuch (a);
		return;
		}

	newdir (x, &s);

	strcpy (buf, tail (a));

	f = d_lookup (buf, &s);

	if (f)
		{
		*f->name = NOENT;
		freechain (f->start);
		s.flag |= DIRTY;

		if (verbose && dflag)
			{
			put ("d ");
			prname (a);
			}
		}
	else
		{
		nosuch (a);
		}

	olddir (&s);
	}

