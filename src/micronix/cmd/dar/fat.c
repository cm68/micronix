/*
 * Having to do the Microsoft disk operating system file
 * allocation table.
 */

# include "dar.h"

chainlength (a)
	unsigned a;
	{
	unsigned b = 0;

	while (a)
		{
		a = nextblock (a);
		b++;
		}

	return b;
	}

/*
 * read in the chain of clusters starting with cluster a
 * starting with memory address b.
 */

readchain (a, b)
	unsigned a;
	char *b;
	{
	extern read ();

	while (a)
		{
		gio (a, b, read);

		a = nextblock (a);
		b += d->bpg;
		}
	}

/*
 * write out the chain of clusters starting with cluster a
 * starting with memory address b.
 */

writechain (a, b)
	unsigned a;
	char *b;
	{
	extern write ();

	while (a)
		{
		gio (a, b, write);

		a = nextblock (a);
		b += d->bpg;
		}
	}

/*
 * galloc - allocate a group or cluster
 * circular allocation scheme is used.
 * The variable n in this routine serves to avoid infinite
 * search when all clusters are allocated.
 * It doesn't hurt much if it's a little too large to begin with.
 * It must be at least the total number of clusters.
 * Ngroup is also the number of the first invalid cluster.
 */

galloc ()
	{
	static g = 3000, n;	/* g starts out wildly out of range */

	for (n = d->ngroup - MINCLUSTER; n; n--, g++)
		{
		if (d->ngroup <= g)	/* force back into range */
			g = MINCLUSTER;

		if (!readfat (g))	/* zero value means unallocated */
			return g++;
		}

	return NULL;	/* could't find one */
	}

/*
 * generate a diskette space report
 */

space ()
	{
	static used, free, percent, g;
	static long l;

	used = 0;
	free = 0;

	for (g = MINCLUSTER; g < d->ngroup; g++)
		{
		if (readfat (g))
			used++;
		else
			free++;
		}

	l = used;
	l *= 100;
	l /= (used + free);

	percent = l;

	put ("Diskette "); putnum (percent), put ("% full, ");

	l = free;	/* number of groups free */
	l *= d->bpg;	/* number of bytes free */
	l /= 1000;	/* number of K free */
	free = l;

	putnum (free), put ("K free space left\n");
	}

/*
 * read an entry from the fat table
 */

readfat (a)
	unsigned short a;
	{
	short b, *p;

	a %= ENTPERFAT;

	p = (short *)(fat + (3 * a) / 2);

	b = *p;			/* pull out word at shifting boundary */

	if (a % 2)		/* odd */
		b >>= 4;	/* take high 12 bits */

	b &= 0xfff;		/* low twelve */

	return b;
	}

writefat (a, b)
	unsigned short a, b;
	{
	short *p;

	a %= ENTPERFAT;

	p = (short *)(fat + (3 * a) / 2);

	if (a % 2)		/* odd */
		{
		b <<= 4;
		b &= 0xfff0;
		*p &= 0x000f;	/* clear high twelve */
		}
	else
		{
		b &= 0x0fff;
		*p &= 0xf000;	/* clear low twelve */
		}

	*p |= b;
	ddirty = YES;
	}

nextblock (a)
	unsigned short a;
	{
	unsigned short b;

	b = readfat (a);

	if (b >= LASTBLOCK)
		return 0;
	else
		return b;
	}

/*
 * zero a block chain in the fat table
 */

freechain (a)
	unsigned a;
	{
	unsigned b;

	while (a)
		{
		b = nextblock (a);
		writefat (a, 0);
		a = b;
		}
	}
