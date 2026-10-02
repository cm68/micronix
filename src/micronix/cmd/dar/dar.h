/*
 * dar - floppy archiver
 * 	DOS - Micronix liason
 *
 *	Len Edmondson
 */

#include <stdio.h>
#include <sys/access.h>
#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <errno.h>
#include <mtab.h>
#include <time.h>
#include <std.h>

/*
 * The original was written against the Whitesmiths types.h and the V6
 * headers.  This tree dropped the Whitesmiths type spellings and
 * renamed the stat fields to the V7 ones, so both are restored here and
 * the .c files stay byte-for-byte the original source.
 */
#define INTERN	static
#define FAST	register
typedef char TEXT, TINY;
typedef int BOOL;
typedef short COUNT;
typedef unsigned char UTINY;
typedef unsigned short UCOUNT;
typedef unsigned long ULONG;

#define S_TYPE	S_IFMT
#define S_ISBLOCK	S_IFBLK
#define S_ISDIR		S_IFDIR
#define flags	st_mode
#define addr	st_addr
#define size1	st_size1

#define lenstr(s)	strlen(s)
#define cmpstr(a,b)	(!strcmp(a,b))
#define cmpbuf(a,b,n)	(!memcmp(a,b,n))
#define cpybuf(a,b,n)	memcpy((a),(b),(n))
#define equal(a,b)	(!strcmp(a,b))
#define element(c,s)	(index((s),(c)) != 0)
#define fill(a,n,c)	memset((a),(c),(n))
#define cfree(p)	free(p)
#define alloc(n)	malloc(n)

char *cpystr();
int itob();
void sort();
void putstr();
extern int errno;

/*
 * some MSDOS diskette layout particulars
 */

#define CLUSTERSIZE	1024	/* maximum size of a cluster */
#define NFAT		2	/* the number of fat tables */

#define FATSTART	1	/* sector number of the beginning of */
 				/* the first file allocation table */

#define FATONE		1
#define FATTWO		2

#define ENTPERFAT	((FATSIZE * 2) / 3)

#define DIRSTART	3
#define DIREND		6

#define LASTBLOCK	0xff8
#define ENDCHAIN	0xfff

/*
 * the lowest valid cluster number
 */

#define MINCLUSTER 2

#define isprint(a) ('!' <= (a) && (a) <= '~')

/*
 * unix ver. 6
 */

#define UENTSIZE 16		/* size of a unix dir. entry */

#define	ESCAPE	'\\'
#define	NEGATE	'^'

#define 	BINARY 	1

#define MTAB "/etc/mtab"
#define RELATIVE 1

#define ALT 8
#define BLOCK 3
#define RECSIZE 128
#define FATSIZE (2 * BUFSIZE)
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
#define MAXENT 112	/* maximum number of dir. entries found on floppies */
#define DIRSIZE 3584
#define PATHSIZE 100	/* maximum file path name length */
#define NAMESIZE 32	/* maximum size for simple file name */

/*
 * settable bits in the attribute member
 * of the entry structure
 *
 */

#define READONLY  (1 << 0)
#define HIDDEN    (1 << 1)
#define SYSTEM    (1 << 2)
#define LABEL     (1 << 3)
#define DIRECTORY (1 << 4)
#define ARCHIVE   (1 << 5)

/*
 * subdirectory buffer states
 */

#define DIRTY (1 << 0)
#define DIRMODE S_IFDIR | 0755


struct entry
	{
	unsigned char
		name [8],
		type [3],
		att,
		res [10];

	unsigned short
		time,
		date,
		start,
		lowsize,
		highsize;
	};

struct disk
	{
	unsigned

/*
 * these goodies have to do with the diskette itself
 */


		bpv,	/* blocks per volume */
		inches,	/* EIGHT or FIVE */
		sides,	/* 1 or 2 */
		dspt,	/* sectors per track DOS */
		uspt,	/* sectors per track UNIX */
		bps,	/* bytes per sector */

/*
 * this information has to do with the underlying DOS formatting
 */

		ngroup,	/* groups per volume */
		bpg,	/* bytes per group */
		spg,	/* sectors per group */
		epg,	/* dir. entries per group */
		epd,	/* dir. entries per directory */
		spd,	/* sectors per directory */

		spf,	/* sectors per fat */
		config,	/* configuration byte */
		dirsec,	/* first sector of directory */
		datasec,/* first sector of data */
		version;
	};

struct subdir
	{
	unsigned size, cluster;
	struct entry *buf;
	char flag;
	};

read (), write ();

extern BOOL	cflag,
	dflag,	/* delete */
	pflag,	/* print */
	rflag,	/* replace */
	tflag,	/* table */
	xflag,	/* extract */
	verbose,
	dbflag,
	alt,
	ddirty,	/* the root directory is dirty */
	complete [];

extern char	**files,
	*device;
extern unsigned char path [];	/* path name buffer */

extern TINY	fd;

extern UCOUNT	nfiles,
	drive;		/* CP/M destination drive */

extern struct entry thedir [MAXENT];

extern unsigned char fat [];

unsigned long entsize ();

extern struct disk *d;

extern struct disk disk [];
