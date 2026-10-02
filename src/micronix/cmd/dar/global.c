
/*
 * dar - dos archiver
 * 	DOS - Micronix liason
 *
 *	Len Edmondson
 */

# include "dar.h"

BOOL	cflag = NO,
	dflag = NO,	/* delete */
	pflag = NO,	/* print */
	rflag = NO,	/* replace */
	tflag = NO,	/* table */
	xflag = NO,	/* extract */
	verbose = NO,
	dbflag = NO,
	alt = NO,
	ddirty = NO,	/* the directory is dirty */
	complete [512] = {0},
	pwd_size = 0,
	pwd_dirty = 0;

TEXT	**files = NULL,
	*device = NULL;
	
TINY	fd = -1;

unsigned char	
	fat [FATSIZE] = {0},	/* file allocation table */
	path [PATHSIZE] = {0};

COUNT	errno;		/* external UNIX error number */

UCOUNT	nfiles = 0;

struct entry thedir [MAXENT] = {0};

struct disk *d = NULL;


struct disk disk []
	=
	{

/* bpv in sides spt pspt bps ngrp bpg spg epg epd spd spf config dirsec datasec */

/*ibm*/

{ 320, 5, 1,  8,  8, 512, 315,  512, 1, 16,  64, 4, 1, 0xfe, 3,  7, 1},
{ 360, 5, 1,  9,  9, 512, 353,  512, 1, 16,  64, 4, 2, 0xfc, 5,  9, 2},
{ 360, 5, 1,  8,  9, 512, 315,  512, 1, 16,  64, 4, 1, 0xfe, 3,  7, 1},

{ 640, 5, 2,  8,  8, 512, 316, 1024, 2, 32, 112, 7, 1, 0xff, 3, 10, 1},
{ 720, 5, 2,  9,  9, 512, 355, 1024, 2, 32, 112, 7, 2, 0xfd, 5, 12, 2},
{ 720, 5, 2,  8,  9, 512, 316, 1024, 2, 32, 112, 7, 1, 0xff, 3, 10, 1},

	{0}
	};

