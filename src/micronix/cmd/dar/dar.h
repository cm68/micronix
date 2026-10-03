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
#include <sys/ioctl.h>
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
 * The DJ/DMA controller's commands, out of include/sys/dj.h.  That header
 * has the block a caller hands the driver and the driver reads it, but dar
 * does not include it: dj.h's own sizes and names are the driver's and are
 * not all dar's, and one of them being a byte off would be a silent
 * misread - the same reason cmd/far spells them out for itself.
 *
 * The block is not a packet the controller decodes but a program it runs:
 * each command is its opcode and its operands, and each leaves what it
 * answered inside itself, which is why the driver hands the block back.
 * ioctl(fd, CDBCMD, &req) runs it - see include/sys/ioctl.h.
 */
#define SREAD	0x20		/* [op,cyl,sec,drv] read a sector */
#define SWRITE	0x21		/* [op,cyl,sec,drv] write one */
#define STATUS	0x22		/* [op,drv] -> dcb,slc,dsb */
#define SETDMA	0x23		/* [op,lo,hi,seg] where the data goes */
#define HALT	0x25		/* [op] the end of the program */
#define MEMREAD	0xA0		/* [op,lo,hi,seg,n,lo,hi,dj] controller memory */

#define VERSO	0200		/* the other side, in a sector number */

#define OKSTAT	0x40		/* what a command that worked left behind */

/*
 * The sector length code a status command answers with, and the sector
 * size each one stands for: 128 bytes, shifted left by the code.
 */
#define S128	0
#define S256	1
#define S512	2
#define S1024	3

/*
 * The controller's own per-drive tables (include/sys/dj.h's DJTAB).
 */
#define DJTAB	0x1340		/* base of the per-drive tables */
#define DJROWSZ	16		/* bytes to a drive's row */

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

extern UCOUNT	nfiles;
extern UCOUNT	drive;		/* which of the board's eight drives the node names */

extern struct entry thedir [MAXENT];

extern unsigned char fat [];

unsigned long entsize ();

extern struct disk *d;

extern struct disk disk [];
