
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
	FAST int c;		/* READ or WRITE */
	{
	static struct cdb req;
	INTERN UCOUNT s, t, n, cyl, sec;

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

	n += t * d->uspt;	/* now n is the linear sector number */

/*
 * What the driver used to do with that linear sector on its way to the
 * board: cylinder, head and sector.  The alternate-sectoring step here is
 * the inverse of the convolution above, so the two cancel and name the
 * sector the medium itself has.
 */

	cyl = n / (d->uspt * d->sides);
	sec = n % d->uspt;

	if (alt)
		{
		sec <<= 1;
		if (!(d->uspt & 1) && sec >= d->uspt)
			sec++;
		sec %= d->uspt;
		}

	sec++;			/* one-origin sector numbers */

	if (d->sides == 2 && (n % (d->uspt * d->sides)) >= d->uspt)
		sec |= VERSO;

/*
 * One ioctl carries the sector, and what it carries is the controller's
 * own program for the transfer: where the data goes, which sector to read
 * or write, and a halt.  The three bytes of the DMA address are the one
 * thing dar cannot write - b is a virtual address and the board wants a
 * physical one - so req.hole says where they go and the driver fills them.
 */

	req.len = 11;		/* SETDMA, the transfer, HALT */
	req.hole = 1;		/* the address is the SETDMA operand */

	req.cmd[0] = SETDMA;
	req.cmd[1] = req.cmd[2] = req.cmd[3] = 0;

	req.cmd[4] = (c == READ) ? SREAD : SWRITE;
	req.cmd[5] = cyl;
	req.cmd[6] = sec;
	req.cmd[7] = drive;
	req.cmd[8] = 0;		/* the transfer's own answer */

	req.cmd[9] = HALT;
	req.cmd[10] = 0;	/* ... and the halt's */

	req.count = d->bps;
	req.flags = (c == READ) ? CDB_IN : 0;
	req.buf = b;

/*
 * The transfer's own answer is in the block that comes back, at the end of
 * the SREAD or SWRITE command - which is where the controller left it, and
 * the reason the block is handed back at all.  An ioctl that worked only
 * says the program ran.
 */

	if (ioctl (fd, CDBCMD, &req) >= 0 && req.cmd[8] == OKSTAT)
		{
		return YES;
		}
	else				/* I/O didn't work */
		{
		perror (device);

		if (c == READ)
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


	if ((s.st_mode & S_TYPE) != S_ISBLOCK)
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
	drive = s.addr[0] & 7;	/* which drive the node names */
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
	static struct cdb req;
	static unsigned char row [DJROWSZ];
	static UCOUNT slc, bps;
	static unsigned volume;

/*
 * The controller is asked, rather than the device measured.  The drive's
 * geometry is one of the controller's own tables (DJTAB), and a command
 * block that reaches into controller memory is how to read it.  There is
 * no command that measures a drive, so this is a program of two: one for
 * the drive's status, one for its row of the table.
 */

/*
 * The drive's status, for the length of a sector on it.  No data phase:
 * the controller leaves its answer inside the command and the driver hands
 * the block back.
 */

	req.len = 6;		/* STATUS */
	req.hole = -1;		/* no address in it to fill */
	req.count = 0;		/* and no data to move */
	req.flags = 0;

	req.cmd[0] = STATUS;
	req.cmd[1] = drive;
	req.cmd[2] = 0;		/* the drive characteristics byte */
	req.cmd[3] = 0;		/* the sector length code */
	req.cmd[4] = 0;		/* the drive status byte */
	req.cmd[5] = 0;		/* and the command's own answer */

	if (ioctl (a, CDBCMD, &req) < 0)
		{
		perror (device);
		exit (NO);
		}

	slc = req.cmd[3];

	if (slc > S1024)
		{
		put ("Unknown sector size\n");
		exit (NO);
		}

	bps = 128 << slc;	/* the code is 128, 256, 512, 1024 */

/*
 * The drive's own row of the controller's table.
 */

	req.len = 10;		/* MEMREAD, HALT */
	req.hole = 1;		/* the address is the MEMREAD operand */
	req.count = DJROWSZ;	/* the whole row */
	req.flags = CDB_IN;
	req.buf = (char *) row;

	req.cmd[0] = MEMREAD;
	req.cmd[1] = req.cmd[2] = req.cmd[3] = 0;
	req.cmd[4] = DJROWSZ;	/* how many bytes to bring back */
	req.cmd[5] = 0;
	req.cmd[6] = (DJTAB + drive * DJROWSZ) & 0xff;
	req.cmd[7] = (DJTAB + drive * DJROWSZ) >> 8;
	req.cmd[8] = HALT;
	req.cmd[9] = 0;		/* the halt's own answer */

	if (ioctl (a, CDBCMD, &req) < 0)
		{
		perror (device);
		exit (NO);
		}

	volume = (row[0] - 1) * row[9];

/*
 * The volume in 512 byte blocks, in the order that does not overflow a 16
 * bit word: a sector is a whole number of 128 byte records, and the volume
 * of them over four is the volume of 512 byte blocks.
 */

	return volume * (bps / 128) / 4;
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
 * c is READ or WRITE
 */

gio (a, b, c)
	FAST UCOUNT a;
	FAST TEXT *b;
	FAST int c;
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

	return (s.st_mode & S_TYPE) == S_ISDIR;
	}

fsize (a)
	{
	struct stat s;

	if (stat (a, &s) < 0)
		return 0;

	return s.size1;
	}

