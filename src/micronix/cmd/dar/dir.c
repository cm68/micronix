/*
 * Having to do with directories.
 */


# include "dar.h"

/*
 * return the size in bytes of the file associated with this entry
 */

	unsigned long
entsize (a)
	struct entry *a;
	{
	return ((unsigned long) (a->highsize) << 16) | a->lowsize;
	}

/*
 * allocate a directory entry
 * stretch the directory if need be.
 */

	struct entry *
ealloc (a)
	struct subdir *a;
	{
	short n, pwd, g;
	struct entry *f, *dir;

	f = a->buf;
	n = a->size;
	pwd = a->cluster;

	for (; n; n--, f++)
		if (*f->name == NOENT || *f->name == 0)
			return f;

/*
 * root dir. doesn't stretch
 */

	if (pwd == 0)
		return NULL;

/*
 * allocate new cluster
 */

	g = galloc ();

/*
 * out of space
 */

	if (g == 0)
		return NULL;

	gzero (g);

	olddir (a);

	n = pwd;

	while (nextblock (n))
		n = nextblock (n);

	writefat (n, g);
	writefat (g, ENDCHAIN);

	newdir (pwd, a);

	return ealloc (a);
	}


/*
 * save a directory in memory
 * a is the cluster number.
 */

newdir (a, b)
	unsigned a;
	struct subdir *b;
	{
	if (a == 0)		/* root directory */
		{
		b->size = d->epd;
		b->buf = thedir;
		}
	else			/* sub directory */
		{
		b->size = chainlength (a) * d->epg;	/* dir size in entries */
		b->buf = calloc (b->size, sizeof (*b->buf));	/* allocate mem. */
		}

	b->cluster = a;
	b->flag = 0;
	readchain (b->cluster, b->buf);
	}

olddir (a)
	struct subdir *a;
	{
	if (a->flag & DIRTY)	/* dir has been changed */
		{
		if (a->cluster == 0)
			{
			ddirty = YES;	/* tranfer to root directory */
			}
		else
			{
			writechain (a->cluster, a->buf);
			}

		a->flag &= ~DIRTY;
		}

	if (a->cluster)		/* sub-dir, mem. image saved */
		{
		cfree (a->buf);
		}
	}


/*
 * read in the directory and the fat
 */

readdir ()
	{
	char *p;
	static s, n;


	if (cflag)		/* create your own fat */
		{
		fill (fat, FATSIZE, 0);
		*fat = d->config;
		fat[1] = fat[2] = ~0;
		ddirty = YES;
		}

	else
		{

/*
 * read in each sector of the fat table
 * if a read fails, try the parallel sector in the second copy of fat
 * The fat always starts in the same place on floppies.
 */


		fatin ();

		if (*fat != d->config && d->uspt == 9)
			{
			d++;
			fatin ();
			}

		if (*fat != d->config)
			{
		
			puts ("Not a DOS diskette.\n");
			exit (1);
			}
		}
	

	if (cflag)
		{
		fill (thedir, sizeof thedir, NOENT);
		}

	else
		{
		dio (READ);
		}
	}


fatin ()
	{
	char *p, s, n;

	p = (char *)fat;
	s = FATSTART;
	n = d->spf;

	while (n--)
		{
		if (!sio (s, p, READ))
			if (!sio (s + d->spf, p, READ))
				return NO;

		p += d->bps;
		s++;
		}
	}


/*
 * write 2 copies of the fat and the directory 
 */

writedir ()
	{
	char *p;
	static s, a, b;

	s = FATSTART;

	for (a = 0; a < NFAT; a++) /* write each fat table */
		{
		p = (char *)fat;

		for (b = 0; b < d->spf; b++)	 /* each sec. of fat table */
			{
			sio (s++, p, WRITE);
			p += d->bps;
			}
		}

	dio (WRITE);		/* write out the directory */
		
	ioflush ();
	}

dio (a)
	int a;			/* READ or WRITE */
	{
	char *p;
	static n, s;

	p = (char *)thedir;	/* first memory address */
	s = d->dirsec;	/* first sector */

	for (n = 0; n < d->spd; n++)
		{
		sio (s, p, a);

		p += d->bps;
		s++;
		}
	}
