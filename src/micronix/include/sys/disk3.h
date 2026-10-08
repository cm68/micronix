/*
 * CompuPro DISK 3 controller interface
 *
 * include/sys/disk3.h
 *
 * The DISK 3 is an intelligent ST-506 hard disk controller: an on board
 * 8085 and its EPROM firmware run the drive, and the host's only real time
 * work is the DMA transfer rate.  All communication is through a single
 * write-only "attention" port and a 16 byte Input Output Parameter Block
 * (IOPB) that lives in main memory: the host fills the IOPB, writes the
 * attention port, and the board copies the IOPB into its own RAM,
 * interprets the command, transfers any data to the address the IOPB
 * names, and writes the completion status back into the IOPB.
 *
 * The port is a pair of addresses (writing either reaches it), switch
 * selectable; the CompuPro standard is 0x90/0x91.  Writing 0xff holds the
 * 8085 in reset; writing 0x00 is the attention pulse that makes it fetch
 * and run an IOPB.
 *
 * The IOPB may begin anywhere in the 24 bit extended address space.  Its
 * first fetch after a reset is from physical 0x50 and is not executed -
 * only the LINK bytes in it are kept - so bring up is: write the LINK at
 * 0x5d, then attention twice.  After that each attention fetches the IOPB
 * the previous one's LINK named, which for a single-IOPB driver is the
 * IOPB itself.
 */
struct disk3_iopb
{
    UINT8 command;              /* opcode, bit 7 = interrupt, bit 6 = continue */
    UINT8 status;               /* 0 = busy; 0xff = done, no error */
    UINT8 drive;                /* logical drive, 0 .. 3 */
    UINT8 arg[7];               /* ARG1 .. ARG7, command dependent */
    UINT8 data;                 /* DMA address, low byte */
    UINT8 xdata;                /* DMA address, middle byte */
    UINT8 sdata;                /* DMA address, high (segment) byte */
    UINT8 link;                 /* next IOPB address, low byte */
    UINT8 xlink;                /* next IOPB address, middle byte */
    UINT8 slink;                /* next IOPB address, high (segment) byte */
};

/*
 * The command byte.  The opcode is the low six bits; the two flags above
 * it say what happens when the command completes.  With CONTINUE set the
 * board fetches the next IOPB from LINK without another attention and the
 * interrupt request is ignored; with it clear the board halts and waits
 * for the next attention.  The interrupt flag is only honoured when
 * CONTINUE is clear.
 */
#define	D3_CONTINUE	0x40	/* fetch the next IOPB, no attention */
#define	D3_INT		0x80	/* interrupt on completion */

/*
 * Operation codes (the low six bits of the command byte).
 */
#define	D3_NOOP		0x00	/* link to a new IOPB, or clear an interrupt */
#define	D3_VERSION	0x01	/* firmware version into ARG1 */
#define	D3_GLOBAL	0x02	/* set mode, retries and drive count */
#define	D3_SPECIFY	0x03	/* load a drive's parameter table from DATA */
#define	D3_SETMAP	0x04	/* load the 256 byte bad sector map from DATA */
#define	D3_HOME		0x05	/* step to track 0 at the low step rate */
#define	D3_SEEK		0x06	/* seek to the cylinder in ARG1-ARG2 */
#define	D3_READHDR	0x07	/* read the next header into DATA */
#define	D3_RW		0x08	/* read or write 1 .. 64K sectors */
#define	D3_RELOCATE	0x09	/* map out a bad track */
#define	D3_FORMAT	0x0a	/* format one track */
#define	D3_FORMATBAD	0x0b	/* overwrite a track with 0x4e, mark it bad */
#define	D3_STATUS	0x0c	/* drive status port into ARG1 */
#define	D3_SELECT	0x0d	/* ARG1 to the drive control ports */
#define	D3_EXAMINE	0x0e	/* dump controller RAM to DATA */
#define	D3_MODIFY	0x0f	/* write controller RAM from DATA */

/*
 * Completion status (the IOPB status byte).  Zero means the board has not
 * finished; 0xff is success.  The rest are error codes.
 */
#define	D3_BUSY		0x00	/* not finished */
#define	D3_ARGERR	0x01	/* something wrong in the IOPB */
#define	D3_NOTRDY	0x02	/* drive not ready */
#define	D3_DATACRC	0x04	/* data CRC error */
#define	D3_WFAULT	0x05	/* write fault */
#define	D3_OVERRUN	0x06	/* overrun on the bus */
#define	D3_HDRCRC	0x07	/* header CRC error */
#define	D3_MAPFULL	0x08	/* bad sector map full */
#define	D3_OK		0xff	/* completed with no errors */

/*
 * The GLOBAL command's mode byte (ARG1): 0xff addresses sectors as one 32
 * bit absolute number, 0 addresses them as a 16 bit sector number within a
 * 16 bit logical track.  The driver runs absolute, which is the only mode
 * that does not have to know the drive's heads.
 */
#define	D3_ABS		0xff	/* ARG2-ARG5 is a 32 bit absolute sector */
#define	D3_LOGICAL	0x00	/* ARG2-ARG3 sector, ARG4-ARG5 logical track */

/*
 * The R/W command (D3_RW).  ARG1 is the direction: 1 reads from the disk,
 * 0 writes to it.  ARG2-ARG5 name the first sector (per the GLOBAL mode)
 * and ARG6-ARG7 the sector count, which must be at least one.  DATA is the
 * 24 bit address the transfer runs through.  On a hard error the arguments
 * are left pointing at the failing sector and count is what remains, so
 * the command can be retried by clearing status and writing attention.
 */
#define	D3_READ		1	/* ARG1: read */
#define	D3_WRITE	0	/* ARG1: write */

/*
 * The SPECIFY command's parameter table, loaded from DATA when a drive is
 * first selected.  Each field is two bytes, low byte first, so the table
 * is 22 bytes.  The driver builds one from the disk label's geometry plus
 * the constants below; the step and settle times are 100 microsecond
 * units, and the firmware ignores the last four bytes.
 */
struct disk3_spec
{
    UINT steprate;              /* seek step time, 100 us units */
    UINT settle;                /* head settle after seek, 100 us units */
    UINT secsize;               /* bytes per sector (512) */
    UINT sectrk;                /* sectors per track */
    UINT trkcyl;                /* heads per cylinder */
    UINT maxtrk;                /* cylinders */
    UINT precomp;               /* first cylinder to precompensate */
    UINT reduce;                /* first cylinder for reduced write current */
    UINT reserved;              /* ignored by current firmware */
    UINT restracks;             /* tracks reserved for bad sector remap */
    UINT position;              /* must be zero */
};

/*
 * The line the board's completion interrupt is on.  The manual's own
 * instructions jumper it to VI1, so that is the line; it is a constant the
 * driver unmask at open and the header names for the dispatcher.
 */
#define	DISK3INT	1

/*
 * The attention port, as CompuPro ships it.  Writing 0xff resets, 0x00
 * attentions; the pair is one port and both addresses reach it.
 */
#define	D3_ATTN		0x90
#define	D3_RESET	0xff	/* the value written to hold the board reset */
#define	D3_GO		0x00	/* the value written to attention */

/*
 * The physical address the first post-reset IOPB is fetched from.  Only
 * its LINK bytes (0x5d-0x5f) are kept; the driver writes them there before
 * the first attention.
 */
#define	D3_INITIOPB	0x50
#define	D3_INITLINK	0x5d	/* the LINK offset within the initial IOPB */
