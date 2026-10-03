# include "dar.h"

/* 
 * replace all the named files
 */

replace ()
	{
	short i;

/*
 * make sure in advance that all the named files
 * are readable
 */

	for (i = 0; i < nfiles; i++)
		if (access (files [i], A_READ) < 0)
			perror (files[i]), exit (NO);


	for (i = 0; i < nfiles; i++)
		{
		if (!repone (cpystr (path, files [i], NULL)))
			return NO;
		}

	return YES;
	}

repone (f)
	char *f;
	{
	if (isdir (path))
		{
		*f++ = '/';
		*f = 0;

		return repdir (f);
		}

	else
		{
		return repfile (path);
		}
	}

/*
 * copy a single file to the dos volume
 * a is the path name
 */


repfile (a)
	char *a;
	{
	char buf [CLUSTERSIZE];
	short in, n;
	struct entry *f;
	short pwd;
	unsigned short new, old;
	unsigned long size;
	struct subdir s;

/*
 * announce
 */

	if (verbose)
		put ("r "), prname (a);


	in = open (a, READ, BINARY);		/* open local file */

	if (in < 0)
		{
		perror (a);
		return NO;		/* open failure */
		}

/*
 * Find the appropriate parent directory on the dos volume.
 * Create a new directory if necessary.
 * 
 * pwd of 0 means the root directory.
 */

	strcpy (buf, parent (a));

	pwd = d_mkdir (buf);

	if (pwd < 0)		/* couldn't make new directory */
		{
		close (in);
		return NO;
		}

	newdir (pwd, &s);

/*
 * see if the file is already there
 */

	f = d_lookup (tail (a), &s);
	
	if (f)		/* overwrite */
		{
		if (f->att & DIRECTORY)
			{
			put (a);
			put (": Is a directory\n");
			close (in);
			olddir (&s);
			return NO;
			}

		freechain (f->start);		/* free old blocks */
		}

	else		/* new file */
		{
		f = ealloc (&s);		/* allocate dir. entry */
		}

	if (!f)	/* there wasn't a parking spot */
		{
		put ("Volume full\n");
		close (in);
		olddir (&s);
		return NO;
		}

	size = 0;
	new = 0;
	old = 0;

	f->start = 0;

	for (;;)
		{
		n = read (in, buf, d->bpg); /* read one cluster's worth */
		
		if (n < 0)
			{
			perror (a);	/* read error */
			freechain (f->start);	/* back out */
			close (in);
			olddir (&s);
			return NO;
			}

		if (n == 0)		/* end of file */
			{
			break;		/* end of file */
			}

		new = galloc ();	/* allocate a group */

		if (!new)
			{
			put ("Not enough space left on volume\n");
			freechain (f->start);
			close (in);
			olddir (&s);
			return NO;
			}

/*
 * record the block number
 */

		if (!old)		/* the first one */
			f->start = new;		/* record in dir. entry */

		else
			writefat (old, new);	/* record in fat */

		writefat (new, ENDCHAIN);	/* terminate the block chain */

		gio (new, buf, WRITE);
		size += n;
		old = new;
		}

/*
 * fill out the directory entry 
 */

	cname (a, f);			/* record name */

	f->date = dosdate ();
	f->time = dostime ();
	f->lowsize = size;
	f->highsize = (size >> 16);

	s.flag |= DIRTY;		/* mark directory buffer written */

	close (in);
	olddir (&s);
	return YES;
	}

/*
 * Copy the directory named in the external array "path"
 * and all its descendents to the dos volume.
 * p is a pointer to the first unused position in the path array.
 */

repdir (p)
	char *p;
	{
	char *b, *q;
	short n, f;

	struct v6dir
		{
		short number;
		char name [14];
		}
	*d;

/*
 * measure local directory.
 */

	n = fsize (path);

/*
 * allocate space for local directory.
 */

	b = calloc (n, 1);

	if (!b)
		return NO;			/* out of memory */

/*
 * open directory in prep. for reading
 */

	f = open (path, READ);

	n = read (f, b, n);		/* read directory */

	close (f);			/* close directory */

# define UENTSIZE 16

	n /= UENTSIZE;		/* scale to v6 dir. entries */

/* 
 * scan directory 
 */

	d = (struct v6dir *)b;

	for (; n > 0; n--, d++)
		{
		if (d->number == 0 || *d->name == '.')
			continue;

		d->name [13] = 0;	/* shorten name */

		if (!repone (cpystr (p, d->name, NULL)))
			{
			cfree (b);
			return NO;
			}
		}		

	cfree (b);

	return YES;
	}
