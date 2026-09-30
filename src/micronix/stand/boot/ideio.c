/*
 * this file is the ide boot code for micronix.
 *
 * The second level of the hard disk boot for a drive on the
 * S100Computers MYIDE board - an 8255 PPI between the S-100 bus and an
 * ATA drive's pins, at ports 0x30 through 0x34.  It is the same protocol
 * as sys/ide.c, and the same card, and the two are meant to be read
 * together; what differs is that this one runs before the kernel, with
 * no buffer cache, no window and no interrupts, and reads one caller's
 * buffer at a time.
 *
 * Terms, the ones that are not just a port number:
 *
 *   task file   the drive's registers, eight of them, addressed by
 *               number: data, error or features, sector count, three
 *               holding an LBA, drive/head, and status or command.  The
 *               card does not map them into the bus, so accessing one is
 *               a short sequence of port writes rather than a load or a
 *               store - that sequence is putreg() below.
 *   LBA         logical block address: the whole disk as one run of
 *               512-byte sectors numbered from 0.  The drive is
 *               addressed this way and is told no geometry at all.
 *   CHS         cylinder, head, sector.  The filesystem still lays a
 *               volume out this way - see dlabel.h - so this file
 *               converts, and that conversion is what lets it read a
 *               volume built for the HD-DMA.
 *   /CS0,/RD,   chip select, read strobe, write strobe and reset.  The
 *   /WR,/RST    leading / marks a signal asserted by pulling it low,
 *               which is why the board inverts most of port C on its way
 *               to the drive and a set bit here asserts the line.
 *   PPI         the 8255 itself.  Three 8-bit ports, A, B and C, whose
 *               direction is set by a mode word written to a fourth
 *               address: ports A and B carry the drive's 16 data lines
 *               and port C its address and control pins.
 *
 * The shape here is mwio.c's, because the filesystem is the same one:
 * open reads the disk label out of the boot cylinder, the geometry comes
 * from there and is guessed nowhere, and the block mapping is the same
 * rotation the kernel uses.  What is missing is everything the HD-DMA
 * needs and this card does not - no channel command block, no sense or
 * recalibrate or load-constants, and no status byte to poll in memory,
 * because the drive reports itself in its own status register.
 */

#include <types.h>
#include <sys/dlabel.h>

struct drivespec {
	UINT cylinders;
	UINT8 heads;
	UINT8 spt;			/* sectors per track */
	UINT limit;			/* max block number */
	UINT spc;			/* sectors per cylinder */
	UINT roll;			/* what idecyl adds to blk / spc */
	UINT cylstart;			/* the slice's first cylinder: the
					 * diskette's boot tracks, 0 on a
					 * rolled disk */
} spec = {
	/*
	 * Nothing.  The geometry comes off the disk and there is no other
	 * source for it - a compiled-in table is a second opinion about a
	 * drive this program has in front of it, and the whole point of
	 * the label is not to need one.  An initialiser of zeroes because
	 * Whitesmith's will not link a bss symbol without one, the same
	 * reason as INIT below.
	 */
	0, 0, 0, 0, 0, 0, 0
};

/*
 * The board's ports.  A port is a Z80 I/O address: a number space of its
 * own, reached with in() and out() rather than by a load or a store.
 *
 * The range is free on this machine: 0x48-0x4f are the Mult I/O console,
 * 0x50-0x53 the obsolete hdca and 0x54-0x55 the HD-DMA.
 */
#define	PA	0x30		/* 8255 port A: drive data, low byte */
#define	PB	0x31		/* 8255 port B: drive data, high byte */
#define	PC	0x32		/* 8255 port C: register address, strobes */
#define	PCTL	0x33		/* 8255 mode word */
#define	PDRV	0x34		/* drive select latch, bit 0 picks unit 1 */

/*
 * 8255 mode words.  Port C is an output either way; what changes is
 * whether ports A and B are inputs - reading a register, or a sector
 * coming off the drive - or outputs, writing one.
 */
#define	PPI_RD	0x92		/* A and B in, C out */
#define	PPI_WR	0x80		/* A, B and C all out */

/*
 * Port C bits.  The register number goes on the low three bits, which
 * are the drive's A0-A2, and the strobes go above it.
 */
#define	C_CS0	0x08		/* chip select, the board's own decode */
#define	C_WR	0x20		/* /WR */
#define	C_RD	0x40		/* /RD */
#define	C_RST	0x80		/* /RST */

/*
 * Task file register numbers.  These are ATA's names and not ports: the
 * board turns one into whatever its bus needs.
 */
#define	DATA	0		/* data register */
#define	ERRST	1		/* error (read) / features (write) */
#define	SCOUNT	2		/* sector count */
#define	LBA0	3		/* LBA 0-7 */
#define	LBA1	4		/* LBA 8-15 */
#define	LBA2	5		/* LBA 16-23 */
#define	DRVHD	6		/* drive/head */
#define	STAT	7		/* status (read) / command (write) */

#define	SBSY	0x80		/* busy */
#define	SDRDY	0x40		/* drive ready */
#define	SDRQ	0x08		/* data request */
#define	SERR	0x01		/* error */

#define	DLBA	0xe0		/* drive/head: LBA addressing, unit 0 */
#define	CREAD	0x20		/* read sector(s), with retry */

#define	SECLEN	512		/* a sector */
#define	NWORD	256		/* and its words, which is what the card moves */
#define	TRIES	10		/* per sector, as the rom's driver retries */

/*
 * Status polls before a command is called failed.  A loop count and not
 * a calibrated interval: the drive has no timeout of its own, and this
 * is sized for one that has been spun down coming back up.  Long enough
 * to ride that out, short enough that a machine with no drive reports an
 * error instead of hanging.
 */
#define	SPIN	20000

/*
 * whitesmith's stupidity means that BSS symbols don't link unless they
 * have an explicit initializer.
 */
#define	INIT	= 0

char tries INIT;

/*
 * Declared before anything calls them: an undeclared call is extern int
 * by default and the definitions say static.
 */
static int idesect(), rdstat(), waitrdy(), waitdrq();
static putreg();

/*
 * reset the drive, and read the label that says how the disk is laid
 * out.
 *
 * Unlike mwio.c there is nothing to prepare: an HD-DMA is a controller
 * that holds state the host put there - a channel command block, a
 * selected drive, a set of constants - and a drive with its own
 * controller holds its own.  So this is a reset to put it in a known
 * state, a wait for it to come out of that, and then the one read that
 * everything else depends on.
 */
reset()
{
	register struct dlabel *lp;
	register int i;
	int sl;

	outstr("Micronix loader for the IDE disk\n");

	out(PCTL, PPI_RD);

	/*
	 * Reset.  /RST is asserted by port C bit 7.  The delay that
	 * follows has to contain a real bus cycle, so it reads port A:
	 * the drive is not driving it, but the read is what keeps the
	 * loop from being dead code.
	 */
	out(PC, C_RST);
	for (i = 0; i < 256; i++)
		in(PA);
	out(PC, 0);		/* /RST released */
	out(PDRV, 0);		/* unit 0 */

	if (!waitrdy()) {
		outstr("Drive not ready\n");
		bail();		/* not exit(): stdio comes in behind it */
	}

	/*
	 * The geometry, out of the label in the boot cylinder.  mkfs
	 * writes it there when it installs a boot, and takes it from the
	 * drive it is installing onto - so the answer is on the disk and
	 * does not have to be guessed.
	 *
	 * Physical cylinder 0, head 0, sector 0 is LBA 0, which is the
	 * one sector a drive can be asked for knowing nothing about it.
	 * That is what the rom read this loader's first level from, and
	 * it is where mkfs writes the label.  It lands in the working
	 * buffer, boot.c's buf0 at 0x1000, which nothing has used yet.
	 */
	tries = 0;
	while (tries++ < TRIES) {
		if (idesect(0, (char *)0x1000))
			break;
	}
	if (tries > TRIES) {
		outstr("label read failed\n");
		bail();
	}

	lp = (struct dlabel *)(0x1000 + DL_OFFSET);
	if (lp->d_magic[0] == DL_MAGIC[0] && lp->d_magic[1] == DL_MAGIC[1] &&
	    lp->d_magic[2] == DL_MAGIC[2] && lp->d_magic[3] == DL_MAGIC[3] &&
	    lp->d_tracks && lp->d_heads && lp->d_spt) {
		spec.cylinders = lp->d_tracks;
		spec.heads = lp->d_heads;
		spec.spt = lp->d_spt;
		spec.spc = lp->d_heads * lp->d_spt;
		spec.roll = lp->d_roll;

		/*
		 * And the slice the boot is in, which on a rolled disk is the
		 * empty one: d_slice[0].d_off is 0, so cylstart is 0 and the
		 * two lines below are exactly what they were before there
		 * were tables.  A disk whose filesystem does not begin at
		 * cylinder 0 - a diskette - says so here, and this is where
		 * the boot tracks get out of the filesystem's way.  See
		 * sys/dlabel.h.
		 */
		sl = lp->d_bootslice;
		if (sl >= NSLICE)
			sl = 0;
		spec.cylstart = lp->d_slice[sl].d_off;
		if (lp->d_slice[sl].d_len)
			spec.limit = lp->d_slice[sl].d_len * spec.spc - 1;
		else
			spec.limit = (lp->d_tracks - spec.cylstart) * spec.spc - 1;
	} else {
		/*
		 * There is nothing to fall back on, and that is deliberate.
		 * A disk that cannot say what it is cannot be read: every
		 * block number goes through the geometry, so guessing it
		 * means reading the wrong cylinder and calling whatever is
		 * there a filesystem.
		 */
		outstr("No disk label at cylinder 0.\n");
		outstr("The geometry is read from there and guessed nowhere.\n");
		outstr("mkfs -i writes one when it installs a boot.\n");
		bail();
	}
}

/*
 * given a block number, read the contents into the buffer
 * return 1 for success
 */
int
readblock(blocknum, buffer)
int blocknum;
char *buffer;
{
	register UINT cyl;
	int secnum;

	if (blocknum > spec.limit) {
		outstr("Block out of range\n");
		return 0;
	}

	/*
	 * Where a block lives, the way sys/ide.c puts it there.
	 *
	 * The block number is a filesystem block and the drive is
	 * addressed by LBA, so this is the conversion between them.  A
	 * cylinder is spc consecutive blocks, so the block number
	 * divides into a cylinder and a position within it - and the
	 * rotation is added on top, because the kernel does not put
	 * block 0 at cylinder 0.  It starts half way round so that the
	 * superblock and the inodes sit in the middle of the platter and
	 * average seek to them halves.  Without the rotation this looks
	 * for the superblock at cylinder 0, reads the boot area instead,
	 * and never finds a filesystem at all.
	 *
	 * spec.spc is a UINT, so blocknum is converted to unsigned
	 * before the division: it has to be, because a large volume's
	 * last block numbers do not fit in a signed int.  spc is also
	 * sectors per *cylinder* whatever the name suggests, so
	 * blocknum / spc is already the cylinder and there is no head
	 * division to do here - the head is inside the LBA.
	 */
	secnum = blocknum % spec.spc;
	cyl = blocknum / spec.spc + spec.roll + spec.cylstart;
	if (cyl >= spec.cylinders)
		cyl -= spec.cylinders;

	tries = 0;
	while (tries++ < TRIES) {
		if (idesect(cyl * spec.spc + secnum, buffer))
			return 1;
		outstr("retry\n");
	}
	outstr("Read error\n");
	return 0;
}

/*
 * One sector, addressed by LBA, into p.  Returns 1, or 0 if the drive
 * refused it or stopped answering.
 *
 * The cylinder and head never reach the drive - it is in LBA mode, and
 * the conversion above is only how a filesystem block became this
 * number.
 */
static int
idesect(lba, p)
UINT lba;
char *p;
{
	register int n;

	/*
	 * The task file.  A command is what makes the drive look at it, so
	 * the order of the registers does not matter; the count of one and
	 * the drive/head of e0h - bit 6 set is LBA mode, unit 0 - are the
	 * whole of what it is told beyond the address.
	 *
	 * The PPI has to be in write mode to put a byte on port A, so it
	 * is switched once at each end rather than once per register.
	 */
	out(PCTL, PPI_WR);
	putreg(DRVHD, DLBA);
	putreg(ERRST, 0);
	putreg(SCOUNT, 1);
	putreg(LBA0, lba & 0xff);
	putreg(LBA1, (lba >> 8) & 0xff);
	/*
	 * LBA 16-23, and it is always zero: a v6 filesystem counts blocks
	 * in a 16-bit number, so no block of any volume this can read
	 * reaches past 65535 and the top byte of the address is never
	 * set.  (sys/ide.c shifts it out of a 32-bit LBA because it has
	 * one; here the value is 16 bits from end to end.)
	 */
	putreg(LBA2, 0);
	putreg(STAT, CREAD);
	out(PCTL, PPI_RD);

	if (!waitdrq())
		return 0;

	/*
	 * And the sector, a 16-bit word at a time, both halves read inside
	 * the one /RD strobe.
	 *
	 * The card latches the next word when the strobe is asserted, not
	 * when a port is read, which is why the loop clears /RD first and
	 * re-asserts it: the leading out() is the pulse's trailing edge,
	 * and without it the drive would step on at the first in() of port
	 * A and hand back the low half of the next word for the high half
	 * of this one.
	 */
	for (n = 0; n < NWORD; n++) {
		out(PC, C_CS0);
		out(PC, C_CS0 | C_RD);
		*p++ = in(PA);
		*p++ = in(PB);
	}
	out(PC, 0);		/* /RD released */
	return 1;
}

/*
 * Write one task file register.  The register number goes on port C with
 * /CS0, the byte on port A, and /WR is pulsed to make the drive take it.
 * /CS0 is held across the access, which is how the drive expects to see
 * it.  The caller has already put the PPI in write mode.
 */
static
putreg(reg, b)
int reg;
int b;
{
	out(PA, b);
	out(PC, reg | C_CS0);
	out(PC, reg | C_CS0 | C_WR);
	out(PC, reg | C_CS0);
}

/*
 * Read the status register.  Register 7 reads as status and writes as
 * command, so the direction is what picks between them.  The PPI is in
 * read mode by the time this is reached.
 */
static int
rdstat()
{
	out(PC, C_CS0 | STAT);
	out(PC, C_CS0 | STAT | C_RD);
	return in(PA);
}

/*
 * Wait for the drive to be out of a command and saying it is ready.  A
 * real drive is busy for a while after a reset, and a task file written
 * while it is would be ignored.
 */
static int
waitrdy()
{
	register int n, st;

	for (n = 0; n < SPIN; n++) {
		st = rdstat();
		if (!(st & SBSY) && (st & SDRDY))
			return 1;
	}
	return 0;
}

/*
 * Wait for the drive to offer the first word of the sector.  Busy and
 * error are both failures here and neither can be waited out, so they
 * end the loop as surely as the data request does; a drive that failed
 * never raises DRQ, so a plain wait for DRQ would end at the same answer
 * having taken longer to get there.
 */
static int
waitdrq()
{
	register int n, st;

	st = 0;
	for (n = 0; n < SPIN; n++) {
		st = rdstat();
		if (st & (SBSY | SERR | SDRQ))
			break;
	}
	return (st & SDRQ) && !(st & (SBSY | SERR));
}
