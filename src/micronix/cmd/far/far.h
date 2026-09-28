/*
 * far - floppy archiver
 * 	CP/M - Micronix liason
 *
 *	Len Edmondson
 *
 * This is a port of Whitesmiths C to the tree's ccc.  The original
 * spelled its storage classes and types with the Whitesmiths
 * pseudo-keywords (TEXT, TINY, COUNT, UCOUNT, UTINY, ULONG, INTERN,
 * FAST, BOOL), which include/std.h keeps disabled - see the comment
 * there.  They are spelled out here as the real types they stood for:
 *
 *	TEXT, TINY	char
 *	COUNT, BOOL	int
 *	UCOUNT		UINT
 *	UTINY		UINT8
 *	ULONG		UINT32
 *	INTERN		static
 *	FAST		register, dropped: ccc's register allocation
 *			has its own rules and nothing here needs it
 *
 * The Whitesmiths library's helpers are gone with it.  element,
 * lenstr, cpystr, putstr, fill, cmpstr, cmpbuf and alloc were its
 * string and memory calls, spelled the other way round from the
 * tree's; they are strchr, strlen, strcpy/strcat, write, memset,
 * strcmp, memcmp and malloc here.
 */

/*
 * stdlib.h has to come before std.h: std.h defines abs() as a macro,
 * and stdlib.h declares "extern int abs(int)", which the macro turns
 * into nonsense if it is already defined.  far calls neither, so it
 * does not matter which wins as long as the declaration gets through.
 */
#include <types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <std.h>
#include <unistd.h>
#include <sys/access.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <mtab.h>

/*
 * The floppy controller's commands, out of include/sys/dj.h.  That header
 * has the block a caller hands the driver and the driver reads it, but far
 * does not include it: dj.h's own sizes and names (K, RECSIZE, BUFSIZE,
 * ALT) are the driver's and are not all far's, and one of them being a
 * byte off would be a silent misread.  cmd/mwformat spells its opcode out
 * for the same reason.
 *
 * The block is not a packet the controller decodes but a program it runs:
 * it takes the command's opcode, does what it says, moves on by the
 * command's own length, and stops at a halt.  Each command is its opcode
 * and its operands, and each leaves what it answered inside itself, at an
 * offset of the command's own - which is why the driver hands the block
 * back to the caller when it is done.
 */
#define SREAD	0x20		/* [op,cyl,sec,drv] read a sector */
#define SWRITE	0x21		/* [op,cyl,sec,drv] write one */
#define STATUS	0x22		/* [op,drv] -> dcb,slc,dsb */
#define SETDMA	0x23		/* [op,lo,hi,seg] where the data goes */
#define HALT	0x25		/* [op] the end of the program */
#define MEMREAD	0xA0		/* [op,lo,hi,seg,n,lo,hi,dj] controller memory */

#define VERSO	0200		/* the other side, in a sector number */

/*
 * What a command that answered without trouble left in its own status byte.
 * A read that did not work answers with something else and leaves the DMA
 * buffer alone, so this is what far has to look at and not the ioctl's
 * return: the driver ran the program it was handed, and running it is not
 * the same as it having read anything.
 */
#define OKSTAT	0x40

/*
 * The sector length code a status command answers with, and the sector
 * size each one stands for: 128 bytes, shifted left by the code.
 */
#define S128	0
#define S256	1
#define S512	2
#define S1024	3

/*
 * The controller's own tables: one row per drive, and a row says how many
 * cylinders the drive has and how many sectors to a cylinder.  Their
 * product is the volume far's format table is counted in, with the first
 * and the last cylinder left out of it (include/sys/dj.h).
 */
#define DJTAB	0x1340		/* base of the per-drive tables */
#define DJROWSZ	16		/* bytes to a drive's row */

#define isprint(a) ('!' <= (a) && (a) <= '~')

#define	ESCAPE	'\\'
#define	NEGATE	'^'

#define MTAB "/etc/mtab"

#define ALT 8		/* the alternate-sectoring bit of a minor number */
#define RECSIZE 128
#define TRACK 15	/* blocks per track */
#define NOENT 0xe5	/* the CP/M untouched-directory byte */
#define NPOINT 8	/* group pointers in an FCB */
#define MAXGROUP 600
#define GENT	64	/* directory entries in a group */
#define MAXGDIR 4	/* directory groups */

/*
 * the 128 byte CP/M record and the sizes built from it.  K is the
 * sector size the directory tables are written in.
 */
#define	R	128
#define	Q	(2 * R)
#define	B	(2 * Q)
#define	K	(2 * B)

#define	K2	(2 * K)
#define	GSIZE	K2	/* size of a group */
#define	K16	(16 * K)
#define	K32	(32 * K)
#define	K64	(64L * K)

/*
 * the size classes the format table is sorted into, in blocks per
 * volume, and mid() names the boundary between two of them.
 */
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

#define	mid(a,b) ((a + b) / 2)

#define	T1	mid(KD,BD)
#define	T2	mid(BD,QD)
#define	T3	mid(QD,KS)
#define	T4	mid(KS,BS)
#define	T5	mid(BS,RD)
#define	T6	mid(RD,D5)
#define	T7	mid(D5,RS)
#define	T8	mid(RS,S5)
#define	T9	mid(S5,H5)

/*
 * an FCB as CP/M writes it.  point is a union because the group
 * pointers are bytes on a small diskette and words on a large one -
 * d->npoint says which, and the same sixteen bytes are read either
 * way.
 */
union fcbpoint {
	UINT	word[NPOINT];
	UINT8	byte[2 * NPOINT];
};

struct fcb {
	UINT8	user,
		name [8],
		type [3],
		ex,
		s1, s2,
		rc;

	union fcbpoint point;
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

/*
 * the program's own state, defined in far1.c
 */
extern int	cflag,
		dflag,	/* delete */
		pflag,	/* print */
		rflag,	/* replace */
		tflag,	/* table */
		xflag,	/* extract */
		verbose,
		alt,	/* the diskette is alternately sectored */
		ddirty;	/* the directory is dirty */

extern int	complete[],	/* the named files that were found */
		gmap[];		/* the groups that are in use */

extern char	**files,	/* the file names on the command line */
		*device;	/* the device node */

extern int	fd;

/*
 * Which of the controller's eight drives the device node names.  It is the
 * low three bits of the node's minor number, and a command block has to
 * say it: the descriptor names the board, and the board has eight drives.
 */
extern UINT	drive;

extern UINT	nfiles,
		userno;

extern struct fcb thedir[];	/* the CP/M directory, in core */

extern struct disk *d;		/* the format the diskette turned out to be */
extern struct disk disk[];	/* every format far knows */

/*
 * far's own functions.  The definitions are old-style, so a name with
 * no return type in front of it returns int - these say int for the
 * same reason, and only the four that hand back a pointer say so.
 */
/*
 * far1.c
 */
extern int init(int, char **);
extern int gio(UINT, char *, int);
extern int sio(UINT, char *, int);
extern int put(char *);
extern int eput(char *);
extern int doflag(char *);
extern int arglist(void);
extern int atmost(void);
extern int finish(void);
extern int main(int, char **);

/*
 * far2.c
 */
extern int mcheck(char *, struct stat *);
extern int fexists(char *);
extern int far(void);
extern int table(void);
extern int tableone(struct fcb *);
extern int name(struct fcb *, char *);
extern int cname(char *, struct fcb *);
extern int extract(void);
extern int exone(struct fcb *, char *);
extern int readex(struct fcb *, int, char *);

/*
 * far3.c
 */
extern int delete(void);
extern int findsize(void);
extern int replace(void);
extern int repone(char *);
extern int exwrite(struct fcb *, int, char *);
extern int findfree(void);
extern int galloc(void);
extern int isfile(char *);
extern int usage(void);

/*
 * far4.c
 */
extern int inter(void);
extern int duptest(void);
extern int iscpm(void);
extern int isempty(void);
extern char *tail(char *);
extern int fdelete(char *);
extern int space(void);
extern char *itoa(int);
extern int putnum(int);
extern int readdir(void);
extern int writedir(void);
extern int match(char *, char *);
extern int omatch(char *, char *);
extern char *nextpat(char *);
extern int alphanum(int);
extern int okchar(UINT8);

/*
 * far5.c
 */
extern int devsize(int);
extern char *save(char *);

/*
 * the Whitesmiths library's names for the tree's - declared here
 * because nothing else spells them this way
 */
extern char *gets(char *);
extern void perror(char *);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
