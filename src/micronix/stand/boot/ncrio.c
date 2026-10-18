/*
 * this file is the NCR 5380 boot code for micronix.
 *
 * Level 3 for a drive on the NCR 5380
 * SCSI host adapter - eight registers at 0x40, driven by hand with no
 * DMA and no arbitration.  It is the same protocol as sys/ncr.c and the
 * same card, and the two are meant to be read together; what differs is
 * that this one runs before the kernel, with no buffer cache, no window
 * and no interrupts, and reads one caller's buffer at a time.
 *
 * Terms, the ones that are not just a port number:
 *
 *   SCSI     a bus, not a drive.  This card is always the initiator and
 *            a disk behind it is always a target, picked by id on the
 *            data lines at selection time.
 *   CDB      command descriptor block: the ten bytes that ARE the
 *            command - an opcode, an LBA and a block count.  READ(10)
 *            is the one this file builds.
 *   LBA      logical block address: the whole disk as one run of
 *            512-byte sectors numbered from 0.
 *   phase    what the bus is being used for at this instant: command,
 *            data in, status, message, then bus free.
 *   REQ, ACK the handshake, one byte at a time: the target raises REQ
 *            when it has a byte or wants one, this card moves the byte
 *            and raises ACK, the target drops REQ, this card drops ACK.
 *
 * The shape here is ideio.c's, because the filesystem is the same one:
 * reset reads the disk label out of the boot cylinder, the geometry
 * comes from there and is guessed nowhere, and the block mapping is the
 * same rotation the kernel uses.  readblock() below is ideio.c's
 * byte for byte; the difference is only the one sector at the bottom.
 */

#include <types.h>
#include <sys/dlabel.h>

struct drivespec {
	UINT cylinders;
	UINT8 heads;
	UINT8 spt;			/* sectors per track */
	UINT limit;			/* max block number */
	UINT spc;			/* sectors per cylinder */
	UINT roll;			/* the slice's own rotation */
	UINT cylstart;			/* the slice's first cylinder */
	UINT len;			/* the slice's length, in cylinders */
} spec = {
	0, 0, 0, 0, 0, 0, 0, 0
};

/*
 * The card's ports, 0x20 through 0x27.  A port is a Z80 I/O address: a
 * number space of its own, reached with in() and out() rather than by a
 * load or a store.  The register number is the low three bits, straight
 * into the chip.
 */
#define CSD	0x20		/* data register, read and write */
#define ICR	0x21		/* initiator command */
#define MR	0x22		/* mode */
#define CSBS	0x24		/* current bus status */
#define RPI	0x27		/* reset parity/interrupt: the ack */

/* the initiator command register's bits */
#define ICR_RST	0x80
#define ICR_ACK	0x10
#define ICR_BSY	0x08
#define ICR_SEL	0x04
#define ICR_ATN	0x02
#define ICR_DATA 0x01

/* the current bus status register's bits */
#define CSBS_BSY 0x40
#define CSBS_REQ 0x20

#define MR_MONBSY 0x04

#define CREAD10 0x28		/* read(10) */
#define CDBLEN	10
#define SECLEN	512
#define TRIES	10
#define SPIN	20000

#define HOSTID	7		/* this card's own id, on the data lines */

/*
 * Whitesmith's stupidity means that BSS symbols don't link unless they
 * have an explicit initializer.
 */
#define INIT	= 0

char tries INIT;

/*
 * The CDB is the ten bytes that are the READ(10) command.  Seven of them
 * never change, so they are a static and ncrread() patches the two LBA
 * bytes in place rather than rebuilding the whole thing every sector.
 */
static char cdb[CDBLEN] = {
    CREAD10, 0, 0, 0, 0, 0, 0, 1, 0, 0
};

/*
 * Declared before anything calls them: an undeclared call is extern int
 * by default and the definitions say static.
 */
static int ncrread(), ncrselect(), ncrbyte();

/*
 * Is the label's magic MWDL?  One load of two words, not four bytes.
 */
static int
hasmagic(lp)
register struct dlabel *lp;
{
	register UINT16 *m = (UINT16 *)lp->d_magic;

	return m[0] == (('W' << 8) | 'M') && m[1] == (('L' << 8) | 'D');
}

/*
 * reset the bus, and read the label that says how the disk is laid out.
 *
 * There is nothing to prepare beyond the bus itself: the 5380 has no
 * channel command block and no selected drive to keep, the target holds
 * its own state.  So this is a bus reset, and then the one read that
 * everything else depends on.
 */
reset()
{
	register struct dlabel *lp;
	register int i;
	int sl;

	outstr("Micronix loader for the NCR 5380 disk\n");

	/* reset the bus and watch BSY come and go */
	out(ICR, ICR_RST);
	for (i = 0; i < 256; i++)
		in(CSBS);
	out(ICR, 0);
	in(RPI);		/* ASSERT RST raised IRQ; acknowledge it */
	out(MR, MR_MONBSY);

	/*
	 * The label, at LBA 0, the one sector a disk can be asked for
	 * knowing nothing about it.  It lands in boot.c's buf0 at 0x1000,
	 * which nothing has used yet.
	 */
	tries = 0;
	while (tries++ < TRIES) {
		if (ncrread(0, (char *)0x1000))
			break;
	}
	if (tries > TRIES)
		bail("label read failed\n");

	lp = (struct dlabel *)(0x1000 + DL_OFFSET);
	if (hasmagic(lp)) {
		spec.cylinders = lp->d_tracks;
		spec.heads = lp->d_heads;
		spec.spt = lp->d_spt;
		spec.spc = lp->d_heads * lp->d_spt;
		sl = lp->d_bootslice;
		if (sl >= NSLICE)
			sl = 0;
		spec.roll = lp->d_slice[sl].d_roll;
		spec.cylstart = lp->d_slice[sl].d_off;
		spec.len = lp->d_slice[sl].d_len;
		if (!spec.len)
			bail("Boot slice is not carved.\n");
		spec.limit = spec.len * spec.spc - 1;
	} else
		bail("No disk label at cylinder 0\n");
}

/*
 * given a block number, read the contents into the buffer
 * return 1 for success
 *
 * The block number is a filesystem block and the drive is addressed by
 * LBA, so this is the conversion between them, the one sys/ncr.c does.
 * spec.spc is a UINT, so blocknum is converted to unsigned before the
 * division - the last block numbers of a large volume do not fit a
 * signed int.
 */
int
readblock(blocknum, buffer)
int blocknum;
char *buffer;
{
	register UINT cyl;
	int secnum;

	secnum = blocknum % spec.spc;
	cyl = blocknum / spec.spc + spec.roll;
	if (cyl >= spec.len)
		cyl -= spec.len;
	cyl += spec.cylstart;

	tries = 0;
	while (tries++ < TRIES) {
		if (ncrread(cyl * spec.spc + secnum, buffer))
			return 1;
		outstr("retry\n");
	}
	outstr("Read error\n");
	return 0;
}

/*
 * One sector, addressed by LBA, into p.  Returns 1, or 0 if the target
 * refused it or stopped answering.
 */
static int
ncrread(lba, p)
UINT lba;
char *p;
{
	register int i;
	char st;

	if (!ncrselect(0))
		return 0;

	/* READ(10), one block at lba: only the two LBA bytes move */
	cdb[2] = lba >> 8;
	cdb[3] = lba;

	for (i = 0; i < CDBLEN; i++)
		if (!ncrbyte(1, &cdb[i]))
			return 0;
	for (i = 0; i < SECLEN; i++)
		if (!ncrbyte(0, p + i))
			return 0;
	if (!ncrbyte(0, &st))		/* the status byte */
		return 0;
	if (!ncrbyte(0, &st))		/* the message byte */
		return 0;
	/*
	 * The bus goes free here, and MONITOR BSY latches that as an
	 * interrupt.  This loader polls and has no handler, so acknowledge
	 * it: a latch left standing is a pending request, and the kernel's
	 * attach loop takes its baseline before the probe runs, so it could
	 * never discover the card's line.
	 */
	in(CSBS);			/* let the target reach bus free */
	in(RPI);
	return 1;
}

/*
 * Call a target and wait for it to answer.  No arbitration: this card
 * takes the bus rather than competing for it, which is legal on a bus
 * with one initiator.  The id bits go on the data lines first, then SEL
 * to say a selection is happening, then BSY to claim the bus; the target
 * answers by raising BSY of its own, and only then do the id bits come
 * off.
 */
static int
ncrselect(target)
int target;
{
	register int n, st;

	for (n = 0; n < SPIN; n++) {	/* the bus has to be free first */
		st = in(CSBS);
		if (!(st & CSBS_BSY))
			break;
	}

	out(CSD, (1 << HOSTID) | (1 << target));
	out(ICR, ICR_DATA);
	out(ICR, ICR_DATA | ICR_SEL);
	out(ICR, ICR_DATA | ICR_SEL | ICR_BSY);

	for (n = 0; n < SPIN; n++) {	/* wait for the target to answer */
		st = in(CSBS);
		if (st & CSBS_BSY)
			break;
	}
	if (!(st & CSBS_BSY)) {
		out(ICR, 0);
		return 0;
	}

	out(ICR, ICR_SEL | ICR_BSY);	/* the id bits come off */
	out(ICR, ICR_BSY);		/* and SEL */
	out(ICR, 0);			/* and our BSY; the target keeps its own */
	return 1;
}

/*
 * One byte, one REQ/ACK round trip: wait for REQ, move the byte, pulse
 * ACK, wait for REQ to drop.  Returns 1, or 0 if REQ never came.
 */
static int
ncrbyte(wr, bp)
int wr;
char *bp;
{
	register int n, st;

	for (n = 0; n < SPIN; n++) {
		st = in(CSBS);
		if (st & CSBS_REQ)
			break;
	}
	if (!(st & CSBS_REQ))
		return 0;

	if (wr) {
		out(CSD, *bp & 0xff);
		out(ICR, ICR_DATA);
		out(ICR, ICR_DATA | ICR_ACK);
		out(ICR, ICR_DATA);
	} else {
		*bp = in(CSD);
		out(ICR, ICR_ACK);
		out(ICR, 0);
	}

	for (n = 0; n < SPIN; n++) {
		st = in(CSBS);
		if (!(st & CSBS_REQ))
			return 1;
	}
	return 0;
}
