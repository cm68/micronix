/*
 * DJ/DMA disk controller
 *
 * include/sys/dj.h
 * Changed: <2021-12-23 15:20:02 curt>
 */

# define	DJINTERVAL	(10 * HERTZ)
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
# define	DISCARD		0xff
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
 * DJDMA command codes
 */


# define	SREAD		0x20
# define	SWRITE		0x21
# define	STATUS		0x22		/* Get drive status */
# define	SETDMA		0x23		/* Set DMA address */
# define	SETINT		0x24		/* Set interrupt */
# define	HALT		0x25		/* Controller halt */
# define	DJHALT		0x25		/* Controller halt */
# define	SETCHANNEL	0x27		/* Set channel address */
# define	SETTRACK	0x2D		/* Set max. track */
# define	MEMREAD		0xA0		/* read controller memory */
# define	MEMWRITE	0xA1		/* write controller memory */
# define	DJEXEC		0xA2

# define	VERSO		0200		/* other side */

# define	STATRET		5

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
