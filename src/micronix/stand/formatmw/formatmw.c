
/*	
 * Format program for Morrow Designs HDC DMA hard disc controller
 *
 *			Version 1.8 (proto)
 *  6 15 84 bjg		Fiddling around with precomp/lowcurrent constants
 *			and added Syquest
 *			Version 1.7 released
 *  9 10 82 Marc	Added check for too many alternate sectors
 *			Version 1.6 released
 *  8 25 82 Marc	Data compare errors do not generate a recalibrate
 *  8 25 82 Marc	Added MiniScribe 2006, 2012, 4010, and 4020 drives
 *  8 24 82 Marc	Added SCO 8-24-82 (driver power up state problems)
 *  8  5 82 Marc	Added AMPEX Pyxis 7, 13, 20, and 27 drives
 *  8  3 82 Marc	Added CMI CM5640 drive
 *  8  1 82 Marc	Added new _main module
 *  7 20 82 Marc	Added recal on hard errors
 *  7 13 82 Marc	Added "nosoft" flag
 *  7 12 82 Marc	Write during testing now REALLY writes, does not format
 * 			Version 1.5 released
 *  6 29 82 Marc	Changed default sector size to 1024 bytes
 *  6 29 82 Marc	Added check for out of range bad map location
 *  6 29 82 Marc	Added the Olivetti HD561/1 drive	
 *  6 28 82 Marc	Added the Olivetti HD561/2 drive
 *			Version 1.4 released
 *  4 30 82 Marc	Cleaned up various drive constants
 *  4 30 82 Marc	Added Seagate st412 and Tandon drives
 *  4 30 82 Marc	Added part number names
 *			Version 1.3 released
 *  3 15 82 Marc	Added MiniScribe 1006 and 1012
 *  3 15 82 Marc	Fixed 'usage' to display version #
 * 12 22 81 Les Kent	Initial coding
 */
	

/*
 * The Whitesmiths run time library this was written against is gone, and
 * what it did provide is not worth chasing: putfmt, sort, btoi/itob and
 * the str/buf routines have no modern equivalent, only near relatives with
 * different contracts.  Everything the program used lives below instead,
 * so the file builds with ccc -m cpm against an ordinary libc.  There is
 * no path back to the original library and no attempt to keep one.
 *
 * Two other things the original spelling could not survive, both in
 * ccc/src/RESTRICTIONS.md: the aggregates it initialised with "= 0" are
 * declared plainly and left to bss, and the sector size switch is scaled
 * because a case label is one byte wide.
 */

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <conio.h>
#include <sys.h>

typedef unsigned char	UTINY;
typedef unsigned int	UCOUNT;

#define	YES		1
#define	NO		0
#ifndef NULL
#define	NULL		0
#endif

#define	iswhite(c)	((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define	out(p, v)	outp((p), (v))

/*
 * CP/M BDOS calls, as the program uses them: 2 is console out, 6 with
 * 0xff is direct console in (0 when nothing is waiting) and 11 is
 * console status.
 */
cpm (fn, arg)
	int fn, arg;
	{
	switch (fn)
		{
	case 2:
		putch (arg);
		return (0);
	case 6:
		return (kbhit () ? getch () : 0);
	case 11:
		return (kbhit ());
		}
	return (0);
	}

/*
 * These four are the library's own code - the contracts are odd enough
 * that reimplementing them from the names would get them wrong.
 * cmpstr is a match predicate, not a difference: it returns YES when the
 * strings are equal, which is why the callers read "if (cmpstr (a, b))".
 */
cmpstr (s1, s2)
	register char *s1, *s2;
	{
	while (*s2)
		if (*s1++ != *s2++)
			return (NO);
	return (*s1 == '\0');
	}

lenstr (is)
	register char *is;
	{
	register char *s;

	for (s = is; *s; ++s)
		;
	return (s - is);
	}

cpybuf (s1, s2, an)
	register char *s1, *s2;
	unsigned an;
	{
	register unsigned n;

	for (n = an; 0 < n; --n)
		*s1++ = *s2++;
	return (an);
	}

/*
 * itob writes the digits without a terminator and returns how many it
 * wrote, so callers punctuate with buf[itob (buf, v, base)] = 0.
 */
itob (is, n, base)
	register char *is;
	int n, base;
	{
	register unsigned u;
	register char *s;

	u = n;
	s = is;
	if (base <= 0)
		base = 10;
	if (base <= u)
		s += itob (s, u / base, base);
	*s = (u % base) + '0';
	if ('9' < *s)
		*s += ('a' - ('9' + 1));
	return (s - is + 1);
	}

/*
 * btoi returns the number of characters consumed, so zero means the
 * argument was not a number at all - that is the caller's error test.
 */
btoi (s, n, ip, base)
	register char *s;
	int n, *ip, base;
	{
	register int val, dig;
	char *is;
	int neg, cnt;

	is = s;
	val = cnt = 0;
	neg = NO;
	for (; 0 < n && iswhite (*s); --n, ++s)
		;
	if (0 < n && (*s == '-' || *s == '+'))
		{
		neg = (*s == '-');
		++s, --n;
		}
	for (; 0 < n; --n, ++s)
		{
		if (*s >= '0' && *s <= '9')
			dig = *s - '0';
		else if (*s >= 'a' && *s <= 'z')
			dig = *s - 'a' + 10;
		else if (*s >= 'A' && *s <= 'Z')
			dig = *s - 'A' + 10;
		else
			break;
		if (base <= dig)
			break;
		val = val * base + dig;
		++cnt;
		}
	if (!cnt)
		return (0);
	*ip = neg ? -val : val;
	return (s - is);
	}

/*
 * The library sort does not move elements itself.  It is handed an
 * ordering function and an exchange function, both taking a pair of
 * indices, and the caller keeps the data wherever it likes - which is
 * why report() passes &order and &ex.  curbad caps at 128, so an
 * insertion sort is the right amount of machinery.
 */
sort (n, ordf, excf)
	int n;
	int (*ordf) ();
	int (*excf) ();
	{
	register int i, j;

	for (i = 1; i < n; i++)
		for (j = i; 0 < j && 0 < (*ordf) (j - 1, j); j--)
			(*excf) (j - 1, j);
	}

/*
 * putfmt is NOT printf, and the difference bites.  From the library's
 * own _putf, the conversion is
 *
 *	%[-|+]<fill>[width][.prec][a|h|o|u]<conv>
 *
 * where '-' left justifies and '+' right justifies, and in both cases
 * the character FOLLOWING the flag is the pad character.  '+' is not a
 * sign flag.  So "%+ 5i" is printf's "%5d" - pad with the space that
 * comes after the '+' - and "%+05i" would be "%05d".
 *
 * The conversions differ too: 'i' is a signed int, and 'p' is a NUL
 * terminated string, printf's %s, not a pointer.  Report() uses both,
 * so getting this wrong silently scrambles the bad sector table.
 */
static
_pfout (s, n)
	register char *s;
	register int n;
	{
	for (; 0 < n; --n, ++s)
		{
		if (*s == '\n')
			cpm (2, '\r');
		cpm (2, *s);
		}
	}

putfmt (char *fmt, ...)
	{
	va_list ap;
	register char *q;
	char buf[24], *s;
	char cfill, mod;
	int rjust, width, prec, base, n;

	va_start (ap, fmt);
	for (q = fmt; *q; )
		{
		if (*q != '%')
			{
			for (s = q; *q && *q != '%'; ++q)
				;
			_pfout (s, q - s);
			continue;
			}
		++q;
		rjust = YES;
		cfill = ' ';
		if (*q == '-')
			{
			rjust = NO;
			cfill = *++q;
			++q;
			}
		else if (*q == '+')
			{
			cfill = *++q;
			++q;
			}
		for (width = 0; isdigit (*q); ++q)
			width = width * 10 + *q - '0';
		prec = 0;
		if (*q == '.')
			for (++q; isdigit (*q); ++q)
				prec = prec * 10 + *q - '0';
		mod = '\0';
		if (*q == 'a' || *q == 'h' || *q == 'o' || *q == 'u')
			mod = *q++;
		base = (mod == 'h') ? 16 : (mod == 'o') ? 8 : 10;

		if (*q == 'p')			/* a string, not a pointer */
			{
			++q;
			s = va_arg (ap, char *);
			n = lenstr (s);
			if (prec && prec < n)
				n = prec;
			}
		else if (*q == 'x')		/* consumes nothing */
			{
			++q;
			s = buf;
			n = 0;
			}
		else if (*q == 'c')
			{
			++q;
			buf[0] = va_arg (ap, int);
			s = buf;
			n = 1;
			}
		else if (*q == 'i' || *q == 's' || *q == 'l' || mod)
			{
			if (*q == 'i' || *q == 's' || *q == 'l')
				++q;
			n = va_arg (ap, int);
			s = buf;
			if (base == 10 && !mod && n < 0)
				{
				buf[0] = '-';
				n = 1 + itob (buf + 1, -n, 10);
				}
			else
				n = itob (buf, n, base);
			}
		else				/* %% and anything unknown */
			{
			s = q++;
			n = 1;
			}

		if (rjust)
			for (; n < width; --width)
				_pfout (&cfill, 1);
		if (0 < n)
			_pfout (s, n);
		if (!rjust)
			for (; n < width; --width)
				_pfout (&cfill, 1);
		}
	va_end (ap);
	}


struct	drives 
	{
	char	*name;	/* Drive name					*/
	int	cyl;	/* Number of cylinders 				*/
	char	heads;	/* Number of heads 				*/
	int	precomp;/* Starting track of precomp 			*/
	int	low;	/* Starting track of low current		*/
	char	stepdl;	/* Step delay (100 uS)				*/
	char	recald;	/* Recal step delay time (100 uS)		*/
	char	settle;	/* Head settle delay (1 MS)			*/
	int	bpt;	/* Bytes per track				*/
	} dtab[]
	=

	{
	{"ampex",	192, 8,   0,  96,   1,  10,   0, 10416},
	{"df516",	192, 8,   0,  96,   1,  10,   0, 10416},
	{"pyxis7",	320, 2,   0, 132,   1,   7,   0, 10416},
	{"pyxis13",	320, 4,   0, 132,   1,   7,   0, 10416},
	{"pyxis20",	320, 6,   0, 132,   1,   7,   0, 10416},
	{"pyxis27",	320, 8,   0, 132,   1,   7,   0, 10416},
	
	{"cmi", 	306, 6,  128, 306,   1,  30,   0, 10416},
	{"m16", 	306, 6,  128, 306,   1,  30,   0, 10416},
	{"cm5616", 	256, 6,  128, 128,   1,  30,   0, 10416},
	{"cm5619", 	306, 6,  128, 306,   1,  30,   0, 10416},
	{"cm5640", 	640, 6,  640, 640,   0,  30,   0, 10416},
	{"m32",		640, 6,  640, 640,   0,  30,   0, 10416},
		
	{"miniscribe",	306, 4,   0, 153,  18,  18,   0, 10416},
	{"ms1006",	306, 2,   0, 153,  18,  18,   0, 10416},
	{"ms1012",	306, 4,   0, 153,  18,  18,   0, 10416},
	{"ms2006",	306, 2,   0,   0,   0,  30,   0, 10416},
	{"ms2012",	306, 4,   0,   0,   0,  30,   0, 10416},
	{"ms4010",	480, 2,   0,   0,   0,  30,   0, 10416},
	{"ms4020",	480, 4,   0,   0,   0,  30,   0, 10416},

	{"olivetti", 	180, 2,  64, 128,  20,  20,   0, 10416},
	{"hd561/1", 	180, 2,  64, 128,  20,  20,   0, 10416},
	{"hd561/2", 	180, 4,  64, 128,  20,  20,   0, 10416},

	{"quantum",	512, 8, 512, 512,   0,  30,   0, 10416},
	{"q2040",	512, 8, 512, 512,   0,  30,   0, 10416},
	{"m34",		512, 8, 512, 512,   0,  30,   0, 10416},

	{"seagate", 	153, 4,  64, 128,  30,  30,   0, 10416},
	{"m5", 		153, 4,  64, 128,  30,  30,   0, 10416},
	{"st506", 	153, 4,  64, 128,  30,  30,   0, 10416},
	{"st412",	306, 4, 128, 128,   0,  30,   0, 10416},
	{"m10",		306, 4, 128, 128,   0,  30,   0, 10416},

	{"tandon",	153, 4,   0, 128,  30,  30,   0, 10416},
	{"tm602",	153, 4,   0, 128,  30,  30,   0, 10416},
	{"tm603",	153, 6,   0, 128,  30,  30,   0, 10416},

	{"syquest",	306, 2, 306, 306,   1,  30,   0, 10416},

	{0},
	};

/*			
 * The following define the number of bytes for sections of the 
 * standard format.
 */

#define	GAP1	16	/* First gap after the index hole		*/
	
#define	SYNC	13	/* Sync field					*/
#define	ID	7	/* Sector header 				*/
#define	GAP2	16	/* Header - data field gap			*/
#define	DATA	4	/* Size of header field less data		*/

#define	GAP4	208	/* 2% for speed variations			*/

#define	FILL	0xe5	/* Data field fill character			*/

/*
 * The following define the defaults if not specified on the command line
 */

#define	DDRIVE	0	/* Default drive				*/
#define	DSIZE	1024	/*  	   Sector size				*/
#define	DTYPE	3	/*	   Type (must match size)		*/
#define	DVER	YES	/* 	   Verify option			*/
#define	DSKEW	3	/*	   Skew factor				*/
#define DTEST	NO	/*	   Drive test routine invocation	*/
#define DSOFT	NO	/*	   Soft map error entry addition	*/
#define DTRACK	-1	/*	   Track number				*/
#define DHEAD	-1	/*	   Head number				*/

/* 
 * Controller commands
 */

#define	READS	0	/* Read a sector				*/
#define	WRITES	1	/* Write a sector				*/
#define	READH	2	/* Read header					*/
#define	FORMAT	3	/* Format a track				*/
#define	LOAD	4	/* Load constants				*/
#define	STATUS	5	/* Read drive status				*/
#define	NOP	6	/* Not much of an OPeration			*/

/*
 * Controller I/O ports
 */

#define	RESET	0x54	/* Reset port (write)				*/
#define	ATTN	0x55	/* Attention port (write)			*/

#define	CHAN	0x50	/* Default channel address			*/

#define	STEPOUT	020	/* Direction bit to step out			*/
#define	STEPIN	0	/*                       in			*/
#define	RETRIES	10	/* Number of retries before giving up		*/
#define	PBIT	0x80	/* Precomp  in drive select			*/
#define	LBIT	0x40	/* Low current bit				*/

/*
 * Status bits 
 */

#define	TRACK0	001	/* Track zero					*/
#define	WFAULT	002	/* Write fault					*/
#define	READY	004	/* Drive ready					*/
#define	SEKCMP	010	/* Seek complete				*/

/*
 * Controller command buffer
 */

struct	cmd
	{		
	UTINY	seksel;		/* Seek select byte		*/
	UCOUNT	steps;		/* Number of steps		*/
	UTINY	sel;		/* Read/Write select byte	*/

	UCOUNT	dma;		/* DMA address			*/
	UTINY	xdma,		/* Extended DMA address		*/

		arg0,		/* Command arguments		*/
		arg1,
		arg2,
		arg3,
		
		cmnd,		/* Command			*/
		stat;		/* Return status		*/
		
	UCOUNT	link;		/* Link address			*/
	UTINY	xlink;		/* Extended link address	*/
	};

struct	cmd	cmd, tcmd;

int	curtrk = -1;

int	pattern[] =
	{
	0x00, 	0xff,
	0x55,	0xaa,
	0x01,	0x02,	0x04,	0x08,	0x10,	0x20,	0x40,	0x80,
	0xfe,	0xfd,	0xfb,	0xf7,	0xef,	0xdf,	0xbf,	0x7f,
	-1,
	};


char	*errors[] =
	{
	"Controller Busy",	/* 0 */
	"Drive not ready",	/* 1 */
	"Wrong cylinder",	/* 2 obsolete (replaced with error 4) */
	"Wrong head",		/* 3 obsolete (replaced with error 4) */
	"Header not found",	/* 4 */
	"Data header not found",/* 5 */
	"Data overrun",		/* 6 */
	"Data CRC",		/* 7 */
	"Write fault",		/* 8 */
	"Header CRC",		/* 9 */
	};

char	gap3[] =
	{
	10,	/* 128 byte sectors 	*/
	18,	/* 256 byte sectors	*/
	43,	/* 512 byte sectors	*/
	64,	/* 1024 byte sectors	*/
	255,	/* 2048 byte sectors	*/
	};

struct	bad
	{
	unsigned int 	track,
			head,
			sector,
			type;
	} bad [128];
	
#define	SOFT	1
#define	HARD	2

int curbad = 0;

#define	BADTRK	0		/* Track, head and sector of bad sector	*/
#define	BADHED	2		/* map.					*/
#define	BADSEC	0

#define	ALTTRK	0		/* Starting track of alternate sectors 	*/
#define ALTHED	2		/* Starting head of alternate sectors 	*/
#define	ALTSEC	1		/* Starting sector of alternate sectors */

#define	IMSIZE	512		/* Image map size 			*/
	
/*
 * buffer is unsigned here because the routines that walk it - tverify,
 * testit, writebad - all declare their cursor as unsigned char *, and a
 * front end fussier than Whitesmiths' calls that a pointer mismatch.
 */
char		image[IMSIZE];	/* Image for sector headers		*/
unsigned char	buffer[2048];	/* Sector buffer			*/

struct	drives *info = &dtab[0];

int	drive 	= DDRIVE,	/* Drive number				*/
	size	= DSIZE,	/* Sector size				*/
	sectyp	= DTYPE,	/* Sector type 0,1,2,3,4		*/
	verify	= DVER,		/* Verify option			*/
	skew	= DSKEW,	/* Skew factor				*/
	test	= DTEST,	/* Test drive flag			*/
	nosoft	= DSOFT,	/* No soft bad map entries		*/
	tracks	= DTRACK,	/* All tracks				*/
	heads	= DHEAD,	/* All heads				*/
	spt     = 0,		/* Sectors per track			*/
	hard	= 0,		/* Number of hard disc errors		*/
	soft	= 0;		/* Number of soft disc errors		*/

/*
 *	Parse CP/M command line arguments
 *
 *	# include <std.h>
 */

# define isblack(a) (!iswhite(a))
# define BUFFER 0x80


main (ac, av)
	int	ac;
	char	*av[];
	{
	struct drives *search;
	int i;
	char *p;

	/*
	 * _main folded the CP/M command tail to lower case on the way in;
	 * crtcpm hands it over as typed, and the drive names are lower case.
	 */
	for (i = 1; i < ac; i++)
		for (p = av[i]; *p; p++)
			*p = tolower (*p);

	ac--; av++;
	
	if (ac == 0)
		usage ();

	search = &dtab[0];
	while (search->name != 0)
		{
		if (cmpstr (av[0], search->name))
			{
			info = search;
			break;
			}
		search++;
		}
	av++; ac--;
	if (!search->name)
		usage ();

	while (ac--)
		{
		if (cmpstr (av[0], "drive"))
			{
			if (ac)
				{
				ac--; av++;
				if (!(btoi (av[0], lenstr (av[0]), &drive, 10))
					|| (drive < 0) || (drive > 3))
						error ("Bad drive arg.");
				av++;
				}
			continue;
			}

		if (cmpstr (av[0], "track"))
			{
			if (ac)
				{
				ac--; av++;
				if (!(btoi (av[0], lenstr (av[0]), &tracks, 10))
					|| (tracks < 0)
					|| (tracks > info->cyl - 1))
						error ("Bad track number.");
				av++;
				}
			continue;
			}

		if (cmpstr (av[0], "head"))
			{
			if (ac)
				{
				ac--; av++;
				if (!(btoi (av[0], lenstr (av[0]), &heads, 10))
					|| (heads < 0)
					|| (heads > info->heads-1))
						error ("Bad head number.");
				av++;
				}
			continue;
			}

		if (cmpstr (av[0], "skew"))
			{
			if (ac)
				{
				ac--; av++;
				if (!(btoi (av[0], lenstr (av[0]), &skew, 10))
					|| (skew < 0) || (skew > 64))
						error ("Bad skew factor.");
				av++;
				}
			continue;
			}

		if (cmpstr (av[0], "size"))
			{
			if (ac)
				{
				ac--; av++;
				btoi (av[0], lenstr (av[0]), &size, 10);
				/*
				 * Switched on size/128 rather than on size.
				 * A case label here is one byte wide, and
				 * 256, 512, 1024 and 2048 all have a low byte
				 * of zero - so written the obvious way they
				 * collapse onto each other and the assembler
				 * says "duplicate case 0".  Divided down they
				 * are 1, 2, 4, 8 and 16, which fit.
				 *
				 * The remainder test keeps the accepted set
				 * exact - 200/128 is 1, and without it a 200
				 * byte sector would be taken for a 128 byte
				 * one.  0 matches no arm and takes default.
				 */
				switch (size % 128 ? 0 : size / 128)
					{
					case 1:
						sectyp = 0;
						break;
					case 2:
						sectyp = 1;
						break;
					case 4:
						sectyp = 2;
						break;
					case 8:
						sectyp = 3;
						break;
					case 16:
						sectyp = 4;
						break;
					default:
						error ("Bad sector size.");
						break;
					}
				av++;
				}
			continue;
			}
		
		if (cmpstr (av[0], "verify"))
			{
			av++;
			verify = YES;
			continue;
			}

		if (cmpstr (av[0], "test"))
			{
			av++;
			test = YES;
			continue;
			}

		if (cmpstr (av[0], "nosoft"))
			{
			av++;
			nosoft = YES;
			continue;
			}

		usage ();
		}

	fmt ();
	if (test)
		{
		dotest ();
		fmt ();
		}
	report ();
	writebad ();
	}

usage ()
	{
	prs ("\nMorrow Designs HDDMA format/test program.  Version 1.7\n\n");
	prs ("Usage: formatmw drive-type [arguments]\n");
	exit ();	
	}

error (mes)
	{
	prs (mes);
	prs ("\n");
	exit (-1);
	}

fmt ()
	{	
	static int hmin, hmax, tmin, tmax;
	static int ctrack, chead;

	reset ();
	spt = (info->bpt-(GAP1+GAP4)) / (SYNC+ID+GAP2+DATA+size+gap3[sectyp]);
 	select ();

	if (tracks == -1)	/* All tracks */
		{
		tmin = 0;
		tmax = info->cyl - 1;
		}
	else
		{
		tmin = tracks;
		tmax = tracks;
		}

	if (heads == -1)
		{
		hmin = 0;
		hmax = info->heads - 1;
		}
	else
		{	
		hmin = heads;	
		hmax = heads;	
		}

	builds ();		/* Build sector numbers */
	cmd.dma = (UCOUNT) image;
	home ();
	prs ("Formatting.\n");
	for (ctrack = tmin; ctrack <= tmax; ctrack++)
		{
		seek (ctrack);
		buildt (ctrack);	
		for (chead = hmin; chead <= hmax; chead++)
			{
			buildh (chead);
			fmthead (ctrack, chead, FILL);
			}
		}
	home ();
	if (verify == NO)
		return;
	/* 
 	 * Verify format
	 */

	prs ("Checking format.\n");
	cmd.dma = (UCOUNT) buffer;
	cmd.xdma = 0;
	for (ctrack = tmin; ctrack <= tmax; ctrack++)
		{
		seek (ctrack);
		for (chead = hmin; chead <= hmax; chead++)
			tverify (ctrack, chead, FILL);
		}
	home ();
	}

/*
 * Build in the sector header image map the sector numbers 
 */

builds ()
	{
	int i, j;

	for (i = 0; i < spt; i++)
		image[i*4+3] = spt + 1;

	for (i = j = 0; j < spt; i = (i + skew) % spt)
		{
		while (image[i*4+3] <= spt)
			i = (i + 1) % spt;
		image[i*4+3] = j++;
		}
	}

/*
 * Build in the sector header image map the track numbers
 */

buildt (track)
	register int track;
	{
	register char *p = image; 

	for (;p < &image[IMSIZE]; p += 4)
		{
		*p = track & 0377;
		*(p+1) = (track >> 8) & 0377;
		}
	}

/*
 * Build in the sector header image map the head numbers
 */

buildh (head)
	int head;
	{
	register char *p = image + 2;

	for (;p < &image[IMSIZE]; p += 4)
		*p = head;
	}

/*
 * Format a track
 */
	
fmthead (track, head, value)	
	register unsigned int track, head;
	register unsigned char value;
	{		

	cmd.seksel 	= drive;			/* Seek select     */
	cmd.steps 	= 0;				/* No steps 	   */
	cmd.sel 	= drive | ((~head & 07) << 2) | LBIT;	

	if (track >= info->precomp)
		cmd.sel |= PBIT;
	if (track >= info->low)
		cmd.sel &= ~LBIT;

	cmd.arg0	= ~(gap3[sectyp] - 1);
	cmd.arg1 	= ~spt;
	cmd.arg2	= ~(size/128 - 1);
	cmd.arg3	= value;
	cmd.cmnd 	= FORMAT;

	if (issue () == NO)
		error ("Format timeout.");

	if (cmd.stat == 0xff)
		return YES;	
	else
		{
		prs (errors[cmd.stat]);
		prs (" error. Track: ");
		prn (track);
		prs (" Head: ");
		prn (head);
		prs ("\n");
		return NO;
		}
	}

/*
 * Issue a command and wait till the command completes
 *
 * Called with a pointer to the return status of a halt.  If the controller
 * does not respond with in a reasonable length of time then the routine
 * returns NO else returns YES.
 */

issue ()
	{
	static unsigned int timeout, timeout2;
		
	timeout  = ~0;	
	timeout2 = 10;
	cmd.stat = 0;
	
	attn ();
	while (timeout2)	
		{
		if (cmd.stat != 0)
			return YES;
		if (!timeout--)	
			timeout2--;
		}	
	return NO;
	}

/*
 * Select drive and check if ready
 */

select ()
	{
	register char *p = (char *) 0x50;

	/*
 	 * Check if controller will execute a halt 
	 */

	*p++ = (unsigned) (&cmd) & 0377;
	*p++ = ((unsigned) (&cmd) >> 8) & 0377;
	*p++ = 0;
	cmd.seksel = drive;
	cmd.steps  = 0;
	cmd.xdma   = 0;
	cmd.sel    = drive | 0x3c;	/* Enable correct driver states */
	cmd.arg0   = 0;
	cmd.arg1   = info->stepdl;
	cmd.arg2   = info->settle;
	cmd.arg3   = (size/128 -1);
	cmd.cmnd   = LOAD;
	cmd.link   = (UCOUNT) &cmd;
	cmd.xlink  = 0;

	if (issue () == NO)
		error ("Controller does not respond.");
		
	cmd.cmnd   = STATUS;

	if (issue () == NO)
		error ("Can't read drive status.");

	if (cmd.stat & READY) 
		error ("Drive not ready.");
	home ();
	}

/*
 *	Recalibrate a drive, used after an error
 */

restore (track)
	int track;
	{
	cpybuf (&tcmd, &cmd, sizeof (cmd));
	reset ();
	select ();
	home ();
	seek (track);
	cpybuf (&cmd, &tcmd, sizeof (cmd));
	}

/*
 * Home the current drive
 */

home ()
	{
	cmd.seksel = drive | STEPOUT;
	cmd.steps  = 0;
	cmd.xdma   = 0;
	cmd.sel    = drive | 0x3c;	/* Enable correct driver states */
	cmd.arg0   = 0;
	cmd.arg1   = info->recald;
	cmd.arg2   = info->settle;
	cmd.arg3   = (size/128 -1);
	cmd.cmnd   = LOAD;

	if (issue () == NO)
		{
		putfmt ("Load constants timeout in home.\n");
		exit ();
		}

	cmd.steps  = 0xffff;
	cmd.cmnd   = NOP;
		
	if (issue () == NO)
		{
		putfmt ("Can't recalibrate drive.\n");
		exit ();
		}
	cmd.steps  = 0;
	cmd.arg1   = info->stepdl;
	cmd.cmnd   = LOAD;

	if (issue () == NO)
		{
		putfmt ("Load constants timeout in home.\n");
		exit ();
		}

	curtrk = 0;		/* Save current track number	*/
 	}

/*
 * Seek to the selected track on the current drive
 */

seek (trk)
	int trk;
	{
	register char *p = (char *) CHAN;

	if (curtrk == -1)
		home ();	/* Home drive if track unknown	*/

	if (curtrk == trk)
		return;		/* Allready at selected track	*/

	cmd.sel  = drive;
	cmd.cmnd = NOP;
	if (trk < curtrk)
		{		/* step out */
		cmd.seksel = drive | STEPOUT;
		cmd.steps  = curtrk - trk;
		}
	else
		{		/* step in */
		cmd.seksel = drive | STEPIN;
		cmd.steps  = trk - curtrk;
		}

	cmd.stat = 0;
	if (issue () == NO)
		{
		putfmt ("Seek timeout\n");
		exit ();
		}

	curtrk = trk;
	}

/*
 * Verify format by reading all sectors and checking data fields
 */

tverify (track, head, data)
	int track, head;
	unsigned char data;
	{
	register unsigned char *sector, *p, retry;
	static unsigned char save;
	static unsigned char value;

	cmd.seksel = drive;
	cmd.steps  = 0;
	cmd.sel    = drive | ((~head & 07) << 2);	/* Select register */
	cmd.arg0   = track & 0377;
	cmd.arg1   = (track >> 8) & 0377;
	cmd.arg2   = head;
	cmd.cmnd   = READS;
	sector 	   = &cmd.arg3;
	
	for (*sector = 0; *sector < spt ; (*sector)++)
		{
		save = 0xff;
		for (retry = RETRIES; --retry;)
			{
			if (issue () == NO)
				{
				prs ("Verify timeout: track: ");
				prn (track);
				prs (" head: ");
				prn (head);
				prs (" sector: ");
				prn (*sector);
				prs ("\n");
 				return;
				}

			if (cmd.stat == 0xff)
				break;

			save = cmd.stat;
			}

		if (save == 0xff)
			continue;
	
		if (save == 7 && !retry)
			{
			for (p = buffer; p < &buffer[size]; p++)
				if (*p != data)
					merr (track, head, *sector, p-buffer);
			}
		else
			verr (save, track, head, *sector, RETRIES-retry-1);
		}
	}

verr (errnum, track, head, sector, count)
	int errnum, track, head, sector, count;
	{
	prs ("Verify: ");
	prs (errors[errnum]);
	prs (" error. Track: ");
	prn (track);
	prs (" Head: ");
	prn (head);
	prs (" Sector: ");
	prn (sector);
	if (count != RETRIES)
		{
		prs (" Count: ");
		prn (count);
		addto (track, head, sector, SOFT);
		}
	else
		{
		prs (" FATAL!");
		restore (track);
		addto (track, head, sector, HARD);
		}
	prs ("\n");
	}

merr (track, head, sector, byte)
	int track, head, sector, byte;
	{
	prs ("Data compare: Track: ");
	prn (track);
	prs (" Head: ");
	prn (head);
	prs (" Sector: ");
	prn (sector);
	prs (" Byte: ");
	prn (byte);
	prs ("\n");
	addto (track, head, sector, HARD);
	}

/*
 * Write a head with a given data pattern
 */

twrite (track, head)
	int track, head;
	{
	register unsigned char *sector, *p, retry;
	static unsigned char save;
	static unsigned char value;

	cmd.seksel = drive;
	cmd.steps  = 0;
	cmd.sel    = drive | ((~head & 07) << 2);	/* Select register */
	cmd.arg0   = track & 0377;
	cmd.arg1   = (track >> 8) & 0377;
	cmd.arg2   = head;
	cmd.cmnd   = WRITES;
	sector 	   = &cmd.arg3;
	
	for (*sector = 0; *sector < spt ; (*sector)++)
		{
		save = 0xff;
		for (retry = RETRIES; --retry;)
			{
			if (issue () == NO)
				{
				prs ("Verify timeout: track: ");
				prn (track);
				prs (" head: ");
				prn (head);
				prs (" sector: ");
				prn (*sector);
				prs ("\n");
 				return;
				}

			if (cmd.stat == 0xff)
				break;

			save = cmd.stat;
			}

		if (save != 0xff)
			werr (save, track, head, *sector, RETRIES-retry-1);
		}
	}

werr (errnum, track, head, sector, count)
	int errnum, track, head, sector, count;
	{
	prs ("Write: ");
	prs (errors[errnum]);
	prs (" error. Track: ");
	prn (track);
	prs (" Head: ");
	prn (head);
	prs (" Sector: ");
	prn (sector);
	if (count != RETRIES)
		{
		prs (" Count: ");
		prn (count);
		addto (track, head, sector, SOFT);
		}
	else
		{
		prs (" FATAL!");
		restore (track);
		addto (track, head, sector, HARD);
		}
	prs ("\n");
	}

/*
 * Issue a controller attention
 */

attn ()
	{
	out (ATTN, 0);
	}

/*
 * Reset controller 
 *
 * Care must be taken that an attention is not issued too soon
 * after the controller has been reset.  User should wait for
 * about 20 micro seconds
 */

reset ()
	{
	out (RESET, 0);
	}

prn (a)
	{
	char buf[5];
	buf[itob (buf, a, 10)] = 0;
	prs (buf);
	}

prb (a)	
	{
	char buf[17];
	buf[itob (buf, a | 0x8000, 2)] = 0;
	prs (&buf[8]);
	}

prs (a)	
	char	*a;
	{
	char c;

	for (;*a;a++)
		{
		if (cpm (11))			/* Console status ready */
			{
			c = cpm (6, 0xff);	/* Read the character */
			if (c == 3)
				exit ();
			if (c == 19)		/* Ctrl S ? */
				{
				while (!(cpm (11)))
					;
				if (cpm (6, 0xff) == 3)
					exit ();
				}
			}
		if (*a == '\n')
			cpm (2, '\r');
		cpm (2, *a);
		}
	}

dotest ()
	{
	int c;
	prs ("Testing disc.\n");

	for (c = 0; pattern[c] >= 0; c++)
		testit (pattern[c], c);
	}

testit (c, pass)
	unsigned char c;
	unsigned int pass;
	{	
	static int hmin, hmax, tmin, tmax;
	static int ctrack, chead;
	register unsigned char *p;

	reset ();
	spt = (info->bpt-(GAP1+GAP4)) / (SYNC+ID+GAP2+DATA+size+gap3[sectyp]);
 	select ();

	if (tracks == -1)	/* All tracks */
		{
		tmin = 0;
		tmax = info->cyl - 1;
		}
	else
		{
		tmin = tracks;
		tmax = tracks;
		}

	if (heads == -1)
		{
		hmin = 0;
		hmax = info->heads - 1;
		}
	else
		{	
		hmin = heads;	
		hmax = heads;	
		}

	prs ("Writing - pass: ");
	prn (pass);
	prs (" data: ");
	prb (c);
	prs ("\n");

	home ();

	cmd.dma = (UCOUNT) buffer;
	cmd.xdma = 0;

	for (p = buffer; p < &buffer[size]; *(p++) = c)
		;

	for (ctrack = tmin; ctrack <= tmax; ctrack++)
		{
		seek (ctrack);
		for (chead = hmin; chead <= hmax; chead++)
			twrite (ctrack, chead);
		}
	home ();
	if (verify == NO)
		return;
	/* 
 	 * Verify format
	 */

	prs ("Reading - pass: ");
	prn (pass);
	prs ("\n");

	cmd.dma = (UCOUNT) buffer;
	cmd.xdma = 0;

	for (ctrack = tmin; ctrack <= tmax; ctrack++)
		{
		seek (ctrack);
		for (chead = hmin; chead <= hmax; chead++)
			tverify (ctrack, chead, c);
		}
	home ();
	}

addto (track, head, sector, type)
	int track, head, sector, type;
	{
	static int count;

	if (type == SOFT && nosoft)
		return;

	count = 0;
	while (count < curbad)
		{
		if ((bad[count].track  == track) &&
			(bad[count].head   == head) &&
			(bad[count].sector == sector))
			{
			if (bad[count].type < type)
				bad[count].type = type;
			return;
			}
		count++;
		}
	bad[count].track = track;
	bad[count].head = head;
	bad[count].sector = sector;
	bad[count].type = type;
	curbad++;
	}

int order (), ex ();

report ()
	{
	int count;

	putfmt ("\n\nBad sector report:\n");

	if (curbad == 0)
		{
		putfmt ("No bad sectors detected.\n");
		return;
		}

	sort (curbad, &order, &ex);
	putfmt ("Track  Head  Sector  Type\n");
	for (count = 0; count < curbad; count++)
		putfmt ("%+ 5i  %+ 4i  %+ 6i  %p\n",
			bad[count].track,
			bad[count].head,
			bad[count].sector,
			(bad[count].type == SOFT) ? "Soft" : "Hard");
	putfmt ("\n");
	}

order (j, i)
	int j, i;
	{
	int value;
		
	if ((value = bad[j].track - bad[i].track) != 0)
		return value;	
	if ((value = bad[j].head - bad[i].head) != 0)
		return value;
	return bad[j].sector - bad[i].sector;
	}

ex (j, i)
	int j, i;
	{
	int track, head, sector, type;

	track  = bad[i].track;
	head   = bad[i].head;
	sector = bad[i].sector;
	type   = bad[i].type;

	bad[i].track  = bad[j].track;
	bad[i].head   = bad[j].head;
	bad[i].sector = bad[j].sector;
	bad[i].type   = bad[j].type;

	bad[j].track  = track;
	bad[j].head   = head;
	bad[j].sector = sector;
	bad[j].type   = type;
	}

/*
 * Write bad sector map out
 */

writebad ()
	{
	static unsigned char *p;
	static int count;
	static int sector;
	static int maxbad;

	if ((BADTRK >= info->cyl) || (BADHED >= info->heads))
		error ("No room for a bad map on this drive.");

	for (p = buffer; p < &buffer[2048];)	
		*p++ = 0;
	
	maxbad = spt * info->heads - spt * ALTHED - ALTSEC;

	if (curbad > maxbad)
		{
		curbad = maxbad;

		putfmt (
"Too many bad sectors.  Assigning alternates for the first %i sectors.\n",
			curbad);
		}

	for (count = 0; count < curbad; count++)
		{
		buffer[count*8+0] = bad[count].track & 0377;
		buffer[count*8+1] = bad[count].track >> 8 & 0377;
		sector = (bad[count].head * spt + bad[count].sector) + 1;
		buffer[count*8+2] = sector & 0377;
		buffer[count*8+3] = sector >> 8 & 0377;
		buffer[count*8+4] = ALTTRK & 0377;
		buffer[count*8+5] = ALTTRK >> 8 & 0377;
		sector = (ALTHED * spt) + ALTSEC + count + 1;
		buffer[count*8+6] = sector & 0377;
		buffer[count*8+7] = sector >> 8 & 0377;
		}
	seek (BADTRK);
	cmd.seksel 	= drive;			/* Seek select     */
	cmd.steps 	= 0;				/* No steps 	   */
	cmd.sel 	= drive | ((~BADHED & 07) << 2) | LBIT;	

	if (BADTRK >= info->precomp)
		cmd.sel |= PBIT;
	if (BADTRK >= info->low)
		cmd.sel &= ~LBIT;

	cmd.dma		= (UCOUNT) buffer;
	cmd.xdma	= 0;
	cmd.arg0	= BADTRK & 0377;
	cmd.arg1 	= BADTRK >> 8 & 0377;
	cmd.arg2	= BADHED;
	cmd.arg3	= BADSEC;
	cmd.cmnd 	= WRITES;

	if (issue () == NO)
		error ("Timeout on writing bad sector map.");

	if (cmd.stat != 0xff)
		error ("Can't write bad sector map.");

	}
