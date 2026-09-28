/*
 * DJ/DMA disk controller
 *
 * include/sys/dj.h
 * Changed: <2021-12-23 15:20:02 curt>
 */

/*
 * The watchdog's period, in resident ticks - the calls sys/ovl.c's
 * ovltick makes, one a second.  This was ten seconds when the driver
 * armed its own timer; the tick the driver is given is a fixed one, so
 * the period is counted here instead.
 */
# define	DJTICKS		10
# define	DJTHRESHHOLD	2

# define	NONE		255
# define	DEFSPECS	8
# define	FIVEBASE	16
# define	MAXTRACK	80

/*
 * per - drive flags bits 
 */

# define 	F_WP		(1 << 2)
# define	ALT		(1 << 3) /* do alternate sectoring */

# define	DOUBSIDE	(1 << 6)
# define	SINGSIDE	0


# define	GETSTAT		(1 << 7)
# define	TYPE		((1 << 4) | (1 << 5))
# define	D		djcomm
# define	IOSTAT		djcomm[8]
# define	INTSTAT		djcomm[10]
# define	SERIAL		0x2C
# define	DISABLE		0
# define	LOGICAL		0x2E
# define	EIGHTFIRST	0
# define	END		10
# define	PIC1		(0x4D)
# define	VI		(1<<1)	/* use int. line 1 */
# define	DJMIN		0x1030
# define	DJMAX		0x127f
# define	DJPRIORITY	PRIBIO

# define	DJINT		1
# define	TIME		 (~0)
# define	MAX		((unsigned) 3000) /* biggest floppy boundary */
# define	CHAN		((unsigned)(djcomm))
# define	OKSTAT		0x40
# define 	NOSTAT		0
# define	BADSTAT		1
# define	COMMSIZE	9
# define	ISOPEN		(1 << 0)
# define	ORG1		(1 << 1) /* sector numbers start from 1 */
# define	MAP		((char *) (0x602))
# define	IMAGE		((char *) (0x202))

/*
 * DJDMA command codes.
 *
 * These are not a packet the board decodes: they are the instructions of
 * a program the board runs.  The controller holds a channel address, and
 * at each command it fetches the byte there, does what it says, leaves a
 * status byte inside the command at a fixed offset, and moves on by the
 * command's own length - until it reaches a HALT, or a BRANCH sends it
 * somewhere else.  That is the whole of the board's control model, and it
 * is why nothing here is a "read a sector of a Micronix filesystem"
 * command: the caller writes a program out of these, and the driver runs
 * it without reading a byte of it.
 *
 * The lengths are not in the table below and cannot be: each one is
 * spelled out at its own define, because the caller has to lay the
 * program out and the board has to step over it, and those two had better
 * agree.  The status offset - always the last byte of the command - is
 * where the controller writes the handler's answer, and it is the byte
 * the caller has to look at.
 *
 * The three at the end are the way out of the instruction set: MEMREAD
 * and MEMWRITE reach any address in the controller's own memory, which is
 * where its per-drive tables live (0x1340, DJTAB), and DJEXEC runs a
 * routine there.  That is how a formatter works - it writes the drive's
 * parameter table and a routine, and runs it - and it is the reason a
 * driver that knows only these commands can format a diskette it does not
 * understand.
 */


# define	SREAD		0x20		/* [op,cyl,sec,drv] read a sector */
# define	SWRITE		0x21		/* [op,cyl,sec,drv] write one */
# define	STATUS		0x22		/* [op,drv] -> dcb,slc,dsb */
# define	SETDMA		0x23		/* [op,lo,hi,seg] where data goes */
# define	SETINT		0x24		/* [op] raise the interrupt line */
# define	HALT		0x25		/* [op] end of the program */
# define	BRANCH		0x26		/* [op,lo,hi,seg] go there next */
# define	SETCHANNEL	0x27		/* [op,lo,hi,seg] where programs start */
# define	SETRETRY	0x28		/* [op,n] error retries */
# define	READTRK		0x29		/* [op,cyl,hd,drv,tab] whole track */
# define	WRITETRK	0x2A		/* [op,cyl,hd,drv,tab] whole track */
# define	SEROUT		0x2B		/* [op,c] the serial line */
# define	SERIN		0x2C		/* [op] ... and the other way */
# define	SETTRACK	0x2D		/* [op,drv,n] tracks on the drive */
# define	SETDRIVE	0x2E		/* [op,p] which drives are the 5" */
# define	SETTIMING	0x2F		/* [op,n] step and settle */
# define	MEMREAD		0xA0		/* [op,src,lo,hi,n,lo,hi,dst] */
# define	MEMWRITE	0xA1		/* [op,src,lo,hi,n,lo,hi,dst] */
# define	DJEXEC		0xA2		/* [op,lo,hi] run a routine there */

/*
 * Where the controller's own tables are, and how long a row is: eight
 * drives, one struct dparam each (djdma.c).  A formatter writes these,
 * and anything that wants to know a drive's geometry reads them rather
 * than being told by the driver - the driver would only be repeating what
 * it read here at open.
 */
# define	DJTAB		0x1340		/* base of the per-drive tables */
# define	DJROWSZ		16		/* bytes to a drive's row */

# define	VERSO		0200		/* other side */

# define	STATRET		5

/*
 * The block a caller hands the driver through ioctl(fd, CDBCMD, &req) is
 * a program out of the commands above, and the driver runs it as it
 * stands.  The driver's own reads and writes are the same thing: sio()
 * lays out a SETDMA, a SREAD or SWRITE, a SETINT and a HALT, thirteen
 * bytes of program in djcomm, and waits for it.  A caller's is its own
 * to write, and longer than those thirteen if it has more to say.
 *
 * Position is physical because the caller is the one that knows the
 * medium.  A Micronix filesystem's sectors are numbered by the drive's
 * table and the driver converts a block number into a track and a
 * sector; a diskette some other system wrote is skewed and laid out in
 * ways that table does not describe, so a caller states the track and
 * sector it wants and the driver does not second-guess it.  The drive is
 * stated the same way, in the command, because the descriptor names the
 * board and the board has eight drives.
 *
 * The address a program's data phase lands at is the one thing the
 * caller cannot write, since buf is a virtual address and the board
 * wants a physical one: req.hole says where in cmd[] the three bytes for
 * it go, and the driver fills them in (include/sys/ioctl.h).
 */

/* 
 * Status byte 1
 */

# define	DOUBLE		0x10		/* 1 if double density */
# define	FIVE		(1 << 2)	/* 1 if 5 1/4 inch drive */
# define	HARD		0x02		/* 1 if hard sectored */

/*
 * Status byte 2
 */

# define	S128		0		/* 128 byte sectors */
# define	S256		1		/* 256 byte sectors */
# define	S512		2		/* 512 byte sectors */
# define	S1024		3		/* 1024 byte sectors */
# define	SHARD		4	/* Hard sectored 256 or 512 byte */

/*
 * Status byte 3
 */

# define	READY		0x80		/* Drive ready bit */
# define	S_WP		(1 << 6)	/* Write protected */
# define	TRACK0		0x20		/* Drive at track zero */
# define	SIDE2		0x04		/* Double sided indicator */

# define	START		0xEF		/* DJ DMA attention port */
# define	DEFCHAN		0x50		/* default chan. addr. */

# define	RECSIZE		128
# define	BUFSIZE		512
# define 	K		1024
# define	K4		4096
# define	UCHAR		unsigned char

# define	NDRIVES		8	

# define	TABADDR		0x1340	 /* base address of tables in 
					    controller memory */


/*
 * drive manufacturer codes
 *     5 1/4" drives
 */

# define TANDON		0
# define SA200		1

# define mS		* 341 / 10

# define STEP		 4		/* offset for step delay */
# define SETTLE		10		/* offset for head settle */


/*
 * delays
 */

struct delay {
	int step, settle;
};

/*
 * the structure of the internal tables of
 * the DJDMA
 */

struct djtable {
	char
    	tracks,		/* no. of tracks on the drive */
		curtrack,	/* current track */
		pattern,
		number;

	int	
        steprate,	/* stepping rate */
		rampup,
		rampdown,
		settle,		/* head settle delay 34.1 to the mS */
		image;

	char	config, code;
};

 
struct status {
	unsigned char	
		other[8],
		spt,
		config,
		dev,		/* device number */
		dchar,		/* drive characteristics */
		slength,	/* sector length */
		dstat,		/* drive status */
		retstat;	/* command return status */
};

struct specs {
	unsigned char
		config,		/* configuration byte */
		ds,		/* double sided (boolean) */
		spt,		/* sectors per track */
		cylinder,	/* secs. per cyl. */
		spb,		/* sectors per block */
		ncyl,		/* number of cylinders */
		toff;		/* track offset */

	unsigned
		bps,		/* bytes per sector */
		volume;		/* secs per disk */
};
		
/*
 * Each drive has one of the following structures
 * associated with it. The structure holds infomation
 * about configuration and current activity.
 */

struct dm {
	struct specs	*specs;
	UCHAR		flags;		/* see below */
};			

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
