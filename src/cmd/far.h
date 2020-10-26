/*
 * far - floppy archiver
 * 	CP/M - Micronix liason
 *
 *	Len Edmondson
 */

#include <stdio.h>
#include <access.h>
/* #include <types.h> */
#include <stat.h>
#include <errno.h>
#include <mtab.h>

#define isprint(a) ('!' <= (a) && (a) <= '~')

#define	ESCAPE	'\\'
#define	NEGATE	'^'

#define MTAB "/etc/mtab"
#define RELATIVE 1

#define ALT 8
#define BLOCK 3
#define RELATIVE 1
#define RECSIZE 128
#define TRACK 15	/* blocks per track */
#define NOENT 0xe5
#define K 1024
#define K2 (2 * K)
#define GSIZE K2	/* size of a group */
#define NPOINT 8
#define K16 (16 * K)
#define K32 (32 * K)
#define K64 (64L * K)
#define MAXGROUP 600
#define GENT	64
#define MAXGDIR 4

#define	R	128
#define	Q	(2 * R)
#define	B	(2 * Q)
#define	K	(2 * B)

#define	KD	2400
#define	BD	2250
#define	QD	1950
#define	KS	1200
#define	BS	1125
#define	RD	 975
#define	D5	 660
#define	RS	 487
#define	S5	 330
#define	H5	 160		/* half size sectors */

#define	T1	mid(KD,BD)
#define	T2	mid(BD,QD)
#define	T3	mid(QD,KS)
#define	T4	mid(KS,BS)
#define	T5	mid(BS,RD)
#define	T6	mid(RD,D5)
#define	T7	mid(D5,RS)
#define	T8	mid(RS,S5)
#define	T9	mid(S5,H5)

#define	mid(a,b) ((a + b) / 2)

struct fcb {
	UTINY	user,
		name [8],
		type [3],
		ex,
		s1, s2,
		rc;

	union {
		UCOUNT	word[NPOINT];
		UTINY	byte[2 * NPOINT];
	} point;
};

struct disk {
/*
 * these goodies have to do with the diskette itself
 */
	unsigned
		bpv,	/* blocks per volume */
		inches,	/* EIGHT or FIVE */
		sides,	/* 1 or 2 */
		spt,	/* sectors per track */
		bps,	/* bytes per sector */

/*
 * this information has to do with the underlying CP/M formatting
 */
		ngroup,	/* groups per volume */
		bpg,	/* bytes per group */
		spg,	/* sectors per group */
		epg,	/* dir. entries per group */
		epd,	/* dir. entries per directory */
		gdir,	/* groups per directory */

		bpe,	/* bytes per entry */
		epe,	/* extents per entry 1 or 2 */
		
		npoint,	/* 8 or 16 */
		offset;	/* sector offset from beginning */
};

read(), write();

BOOL	cflag,
	dflag,	/* delete */
	pflag,	/* print */
	rflag,	/* replace */
	tflag,	/* table */
	xflag,	/* extract */
	verbose,
	alt,
	ddirty,	/* the directory is dirty */
	complete [],
	gmap [];

TEXT	**files,
	*device;
	
TINY	fd;

COUNT	errno;		/* external UNIX error number */

UCOUNT	nfiles, 
	userno;

struct fcb thedir[];

struct disk *d;

struct disk disk[];

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */

