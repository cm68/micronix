/*
 * HD-DMA Controller command structure
 *
 * include/sys/mw.h
 * Changed: <2021-12-23 14:28:50 curt>
 */
struct hddma_cmd
{
    UINT8 drvsel;               /* out<<4 | drv */
#define STEPOUT		0x10		/* step out toward track 0 */
    UINT steps;                 /* number of steps */
    UINT8 headsel;              /* pcmp<<7 | hicur<<6 | (~head&7)<<2 | drv */
#define PRECOMP		0x80		/* Write precompensation */
#define HIGHCUR		0x40		/* Use high write current */
    UINT dma;                   /* dma address */
    UINT8 xdma;                 /* high byte of 24-bit address */

    union {
        UINT word;
        struct {
            UINT8 low;
            UINT8 high;
        } byte;
    } arg0;
    UINT8 byte2;
    UINT8 byte3;

    UINT8 opcode;               /* op code */
    UINT8 status;               /* completion status */
#define	SENSE_TRK0		0x01	/* zero if trk0 */
#define	SENSE_WFAULT	0x02	/* zero if wfault */
#define	SENSE_READY		0x04	/* zero if ready */
#define	SENSE_SEEKDONE	0x08	/* zero if seek done */
#define	SENSE_INDEX		0x10	/* toggles each rotation */
    UINT link;                  /* address of next command */
    UINT8 xlink;                /* high byte of address */
};

/*
 * The four argument bytes, and what a command makes of them.  A read or a
 * write matches them against the sector header it is looking for -
 * cylinder low, cylinder high, head, sector - and a format carries the
 * shape of the track instead: gap3, the sectors on the track, the code
 * for the sector size, and the byte every data field is filled with.  The
 * board counts up to overflow rather than down to zero, so the count and
 * the size code travel negated (hwsim/d1/hddma.c decodes them that way).
 *
 * byte0 and byte1 name the halves of the cylinder word, which is also
 * arg0 below; byte2 and byte3 are the two bytes after it.
 */
#define	byte0	arg0.byte.low
#define	byte1	arg0.byte.high
#define	word0	arg0.word

#define	gap3	arg0.byte.low	/* ~(gap length - 1) */
#define	sptneg	arg0.byte.high	/* ~sectors per track */
#define	fseccode byte2		/* ~(sector size / 128 - 1) */
#define	fill	byte3		/* the data field's fill byte */

/*
 * The line the controller's completion interrupt is on, as the kernel
 * numbers them.  It is bus vector VI0, which is where the Decision 1's
 * jumper area puts the hard disk controller (sys/ide.c has the note on
 * the three lines a card on the bus can reach).  The driver unmasks it
 * at open with inton(MWINT), and its header names it for the interrupt
 * dispatcher - the header is a different object (sys/mwhdr.c), so the
 * number is here rather than beside the driver.
 */
#define	MWINT	0

/*
 * Controller commands
 */
#define OP_READ			0       /* read sector */
#define OP_WRITE        1		/* write sector */
#define OP_HEADER       2		/* read header */
#define OP_FORMAT		3       /* format */
#define OP_LOAD			4       /* load constants */
#define OP_SENSE		5       /* get drive status */
#define OP_NOP			6		/* noop - recalibrate */

/*
 * Controller constants
 */
#define HOMDEL  30              /* step pulse delay during home in 100 us */
#define SETTLE  0               /* controller head-settle time */
#define INT     0x80            /* Interrupt enable bit with step delay */
#define	SEC512	3               /* 512 byte sectors */

/*
 * port addresses 
 */
#define HDC_CCA     0x50        /* default channel command address */
#define HDC_RESET   0x54        /* Reset to controller */
#define HDC_ATTN    0x55        /* Attention to controller */

#define LCONST		0x30    /* must be set for LOAD constants command */
#define NOTRDY		4       /* bit 2 of sense status */
#define BUSY		0       /* controller is busy */
#define OK		0xff    /* completed without error: the status a good
				 * command leaves in the block, and the one
				 * thing a caller of the raw command ioctl has
				 * to check for itself */
#define INTOFF		0       /* turn off completion interrupts */

/* status bits */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
