/*
 * routines having to deal with the msdos file system
 */


# include "dar.h"

d_fexists (a)
	char *a;
	{
	return d_namei (a) >= 0;
	}

/*
 * turn a path name into a cluster number
 */

	unsigned
d_namei (a)
	char *a;
	{
	return d_stat (a, 0);
	}

/*
 * Return <0 on failure 
 * Obtain the directory entry for a file
 */

	unsigned
d_stat (a, statbuf)
	char *a;
	struct entry *statbuf;
	{
	char comp [NAMESIZE], buf [NAMESIZE];
	unsigned n, i;
	struct entry *dir, *f;
	unsigned char c;
	struct subdir s;


	n = 0;		/* start with the root directory */

	for (;;)
		{
		a = getcomp (a, comp);

		if (!*comp)
			{
			return n;
			}

		if (cmpstr (comp, "."))
			{
			continue;
			}

/*
 * We need to deal with .. eventually.
 */

		newdir (n, &s);

		f = d_lookup (comp, &s);

		if (f)			/* found it */
			{
			n = f->start;

			if (statbuf)
				cpybuf (statbuf, f, sizeof *statbuf);
			}

		else
			{

			return -1;
			}

		olddir (&s);
		}
	}

/*
 * d_isdir - does the given string represent a directory on
 * the dos volume ? 
 */

d_isdir (a)
	char *a;
	{
	char *t;
	char buf [NAMESIZE];
	int n;
	struct entry *f;
	struct subdir s;

	t = tail (a);

	if (*t == '.')
		return YES;

	strcpy (buf, parent (a));

	n = d_namei (buf);

	if (n < 0)
		return NO;	/* No such file or directory. */

	newdir (n, &s);
	
	f = d_lookup (t, &s);

	olddir (&s);

	return f && (f->att & DIRECTORY);
	}

/*
 * Create a new directory on a dos volume.
 * Don't complain if it already exists.
 * Also create all the ancestral directory components
 * leading to the desired directory.
 * Return the new directory's cluster number.
 * Sometimes a subdirectory will need to be stretched and
 * another cluster pointer added to its chain.
 * Return (-1) on failure.
 * Causes of failure:
 * Out of root directory space
 * Out of clusters.
 * This is a recursive routine.
 */

d_mkdir (new)
	char *new;
	{
	char buf [PATHSIZE];
	short p, q;
	struct entry *f;
	struct subdir s;

	if (!*new || equal (new, "/") || equal (new, "."))
		return 0;		/* root cluster */

/*
 * make your parent first, then make yourself
 */

	strcpy (buf, parent (new));

	if (d_mkdir (buf) < 0)		/* pass on failure condition */
		return -1;

	q = d_namei (new);

	if (q >= 0)
		return q;		/* all done */

/* create directory */

/*
 * find the cluster number of the parent directory 
 */

	strcpy (buf, parent (new));

	p = d_namei (buf);

	if (p < 0)
		{
		nosuch (new);
		return -1;		/* failure */
		}

/*
 * allocate a new entry in the parent directory
 */

	newdir (p, &s);
	f = ealloc (&s);

	if (!f)
		{
		put (new);
		put (": Can't allocate directory entry\n");
		olddir (&s);
		return -1;
		}

/*
 * we need to allocate a cluster.
 * This new cluster number will become the identity of the
 * new directory.
 */

	q = galloc ();

	if (!q)
		{
		put (new);		
		put (": Volume full\n");
		olddir (&s);
		return -1;
		}

	gzero (q);
	writefat (q, ENDCHAIN);

/*
 * fill out the principal directory entry for the new
 * sub directory.
 */

	fill (f, sizeof *f, 0);

	cname (tail (new), f);

	f->att = DIRECTORY;
	f->start = q;
	f->date = dosdate ();
	f->time = dostime ();
	s.flag |= DIRTY;		/* mark the directory dirty */
	olddir (&s);

/*
 * Now add entries for . and ..
 */

	newdir (q, &s);

	f = ealloc (&s);

	if (f)
		{
		fill (f, sizeof (*f), 0);
		cname ("", f);
		*f->name = '.';
		f->att = DIRECTORY;
		f->start = q;
		f->time = dostime ();
		f->date = dosdate ();
		}
/*
 * ..
 */


	f = ealloc (&s);

	if (f)
		{
		fill (f, sizeof (*f), 0);
		cname ("", f);
		f->name [0] = '.';
		f->name [1] = '.';
		f->att = DIRECTORY;
		f->start = p;
		f->time = dostime ();
		f->date = dosdate ();
		}

	s.flag |= DIRTY;
	olddir (&s);

/*
 * return the new cluster number
 */

	return q;
	}

/*
 * search for the name a in directory b
 */

	struct entry *
d_lookup (a, b)
	char *a;
	struct subdir *b;
	{
	unsigned char c;
	char buf [BUFSIZE];
	int i;
	struct entry *f, e;

	cname (a, &e);

	i = b->size;
	f = b->buf;

	for (; i; i--, f++)
		{
		c = *f->name;
	
		if (c == 0 || c == NOENT || c == '.')
			continue;	/* empty slot */

		if (cmpbuf (&e, f, sizeof f->name + sizeof f->type))
			return f;
		}

	return NULL;		/* not found */
	}
