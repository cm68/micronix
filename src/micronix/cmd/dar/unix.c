
/*
 * dar - dos archiver
 * 	DOS - Micronix liason
 *
 *	Len Edmondson
 */

# include "dar.h"


/*
 * sio - sector I/O
 *	read or write 1 sector
 */

sio (a, b, c)
	FAST UCOUNT a;
	FAST TEXT *b;
	FAST (*c)();
	{
	INTERN UCOUNT s, t, n;
	INTERN ULONG o;



	s = a % d->dspt;		/* sector no. on the track */
	t = a / d->dspt;		/* track no. */

/*
 * UNIX convolution
 */


	if (d->uspt == 8)	/* DOS 1.00, 8 spt, 1 side */
		{
		if (alt)
			n = s * 4 + s / 2;
		else
			n = s;
		}

	else if (d->uspt == 9)	/* DOS 2.00, 9 spt, 1 side */
		{
		if (alt)
			n = s * 5;
		else
			n = s;
		}


	n %= d->uspt;	/* bring into the range [0, spt] */

	n += t * d->uspt;	/* now n is the real sector number */



	o = n;		/* offset in sectors */

	o *= d->bps;	/* o is the offset in bytes */

	lseek (fd, o, 0);	/* seek to the spot */

	if ((*c) (fd, b, d->bps) == d->bps)
		{
		return YES;
		}
	else				/* I/O didn't work */
		{
		perror (device);

		if (c == read)
			fill (b, d->bps, NOENT);

		return NO;
		}
	}

/*
 * produce an argument list from the file names in the current directory
 * under unix 
 */

arglist ()
	{
	extern char **files;
	extern unsigned nfiles;
	static f;
	static struct stat s;

	struct dir
		{
		int number;		
		char name [16];
		};

	static struct dir d;

	if (rflag == NO || nfiles > 0)
		return;

	f = open (".", READ);

	if (f < 0)
		return;

	fstat (f, &s);		/* measure the directory */

	files = calloc (s.size1 / 16, sizeof *files);
	nfiles = 0;

	for (;;)
		{
		if (read (f, &d, 16) != 16)
			break;
		
		if (d.number == 0 || *d.name == '.')
			continue;

		files[nfiles++] = save (d.name);
		}

	close (f);
	}


okdev ()
	{
	static struct stat s;

	if (stat (device, &s) < 0)
		{
		perror (device);
		return NO;
		}

/*
 * make sure we are dealing with a block special file
 */


	if ((s.flags & S_TYPE) != S_ISBLOCK)
		{
		put (device);
		put (": Not block special\n");
		return NO;
		}

	return YES;	/* no reason why not. */
	}

/*
 * make sure the given arg does not refer to a mounted floppy
 */

mcheck (a, s)
	char *a;
	struct stat *s;
	{
	char *p, buf [32], f;
	struct mtab mtab;
	struct stat st;

/*
 * read the mount table, statting each entry
 */

	f = open (MTAB, READ);

	if (f < 0)		/* can't open mtab */
		return;

	for (;;)
		{
		static n;

		n = read (f, &mtab, sizeof (mtab));

		if (n != sizeof (mtab))
			break;		/* end of file */

		if (!*mtab.special || !*mtab.directory)
			continue;	/* unused entry */

/*
 * build full path name
 */

		cpystr (buf, "/dev/", mtab.special, NULL);

		if (stat (buf, &st) < 0)
			continue;

		if (st.addr[0] != s->addr[0])
			continue;	/* different major dev no. */

		errno = EBUSY;
		perror (mtab.special);
		exit (NO);
		}

	close (f);
	}

/*
 * does the given file exist ?
 */

fexists (a)
	{
	struct stat s;

	return stat (a, &s) >= 0;
	}

/*
 * open the device, set fd
 */

devopen ()
	{
	INTERN mode;
	struct stat s;

	if (fd >= 0)
		return;		/* already open */

	if (dflag || rflag)
		mode = UPDATE;	
	else
		mode = READ;

	fd = open (device, mode);	/* open the device */

	if (fd < 0)
		fd = open (device, mode);	/* try twice */

	if (fd < 0)
		perror (device), exit (NO);

	fstat (fd, &s);

	alt = !!(s.addr[0] & ALT);
	}

changename ()
	{
	INTERN TEXT buf [32];

/*
 * if the device name was given as simple file name
 * see if prepending /dev/ will make things better
 */

if (!fexists (device) && !element ('/', device) && lenstr (device) < 16)
		{
		cpystr (buf, "/dev/", device, NULL);
		device = buf;
		}
	}

create (a)
	char *a;
	{
	return creat (a, 0666);
	}

devsize (a)
	{
	char c;
	long low, high, midpoint;


	low = 0;
	high = K64;

	for (;;)
		{
		midpoint = (low + high) / 2;

		if (midpoint == low)
			break;

		if (seek (a, (int) midpoint, 3) >= 0 && read (a, &c, 1) == 1)
			{
			low = midpoint;
			}

		else
			{
			if (errno == ENXIO)	/* off the disk */
				{
				high = midpoint;
				}
			else		/* on the disk but unreadable */
				{
				perror (NULL);
				low = midpoint;
				}
			}
		}

	seek (a, 0, 0);	/* return the file offset to the beginning */

	if (high >= K64)
		high = K64 - 1;		/* max. size */

	return (int) high;
	}


/*
 * flush the disk system
 */

dflush ()
	{
	}

/*
 * read or write 1 cluster or group
 * a is the cluster number
 * b is the memory address
 * c is &read() or &write()
 */

gio (a, b, c)
	FAST UCOUNT a;
	FAST TEXT *b;
	FAST (*c)();
	{
	char *p;
	int i, s, r;



	r = YES;		/* ok so far */

	p = b;			/* initial address */

/*
 * 1. Subtract 2 from the cluster number
 *
 * 2. Multi;ly the result by the number of sectors 
 *    per cluster.
 *
 * 3. Add the logical sector number of the
 *    beginning of the data area.
 */


	s = (a - 2) * d->spg + d->datasec;

	for (i = 0; i < d->spg; i++)
		{
		if (!sio (s, p, c))
			r = NO;

		p += d -> bps;
		s++;
		}

	return r;
	}

/*
 * standard environment ? 
 */

standard ()
	{
	char buf [BUFSIZE];

	return gtty (STDIN, buf) >= 0 && gtty (STDOUT, buf) >= 0;
	}


getdevname ()
	{
	for (;;)
		{
		put ("Insert diskette and close the drive door\n");

		put ("Enter disk name: ");

		gets (device);

		if (feof (stdin))
			exit (YES);

		changename ();

		if (!okdev ())
			continue;

		break;
		}
	}

ioflush ()
	{
	}


readlocal (a)
	char **a;
	{
	return 0;
	}
		
isdir (a)
	char *a;
	{
	struct stat s;

	if (stat (a, &s) < 0)
		return NO;

	return (s.flags & S_TYPE) == S_ISDIR;
	}

fsize (a)
	{
	struct stat s;

	if (stat (a, &s) < 0)
		return 0;

	return s.size1;
	}

