/*
 * this file is the DJ-DMA boot code for micronix.
 *
 * stand/boot/djio.c
 *
 * The device driver half of the floppy boot, the stand-in for mwio.c.
 * The filesystem code (boot.c) is shared with the hard disk boot; only
 * this file changes, and only in how a block is read.
 *
 * The DJ-DMA is not the HD-DMA.  The HD-DMA takes a sector, a DMA
 * address and an opcode in a command block at 0080 and the host polls a
 * status byte.  The DJ-DMA has its own z80 - the DJDMA25 firmware - that
 * runs a channel program out of the host's memory.  The host writes a
 * SETDMA / SREAD / HALT sequence at the default channel address (0050),
 * pulses the attention port (00ef) and polls the status byte the
 * controller writes back.
 *
 * The geometry is not fixed and never was, it was guessed: one floppy is
 * 40 cylinders by two heads by ten sectors and another is 77 by one by
 * fifteen, and this file used to hold the first of those as constants.
 * It reads the medium off the disk instead, out of the label in the
 * block the rom read us from - the same struct dlabel a hard disk
 * carries, saying the same kind of thing.  See sys/dlabel.h.
 */
#include <types.h>
#include <sys/dlabel.h>

/*
 * The DJ-DMA channel.  The program sits at the default channel command
 * address and the attention port starts it.
 */
#define	CCA	0x50		/* the default channel command address */
#define	ATTN	0xef		/* pulse to start the channel */

#define	SETDMA	0x23		/* set the 24 bit DMA address */
#define	SREAD	0x20		/* read one sector */
#define	HALT	0x25		/* end the channel program */
#define	OKSTAT	0x40		/* a command's good status */

#define	INIT	= 0

/*
 * Fixed addresses, reached through a pointer rather than as
 *
 *	*(int *)CCA = ...;
 *
 * for the reason mwio.c gives: the compiler drops a store through a
 * cast constant and says nothing.  The channel program is 11 bytes -
 * SETDMA (4), SREAD (5), HALT (2) - and the parts that do not change
 * from one read to the next are written once, in reset().
 */
UINT8 *chan = (UINT8 *)CCA;

/*
 * Declared before anything calls it: an undeclared call is extern int by
 * default and the definition says static.
 */
static int djrsec();

char tries INIT;
int curcyl INIT;

/*
 * The medium, out of the label: what a block number means on this disk.
 *
 * cylstart is the label's whole point on a floppy.  dj.c maps a block
 * to a cylinder by adding a fixed track offset and never rotating, so
 * the boot the rom reads occupies the cylinders in front of the
 * filesystem and no filesystem block number reaches them - which is
 * both where the boot is and why the filesystem needs no boot file
 * inside it.  cylstart is that offset, and it is the slice idiom's
 * d_slice[d_bootslice].d_off.  A rolled disk's slice table is empty, so
 * this reads 0 and the arithmetic below is what it always was.
 *
 * limit is the last block number that exists, so a diskette whose label
 * describes a smaller medium than the drive holds cannot be read off
 * the end.  It is the slice's own bound when it has one and the whole
 * medium when it does not.
 *
 * firstsec is the number of the first sector on a track, and it is a
 * third kind of thing again: not where the blocks start, not what shape
 * they are in, but what the medium calls them.  A soft sectored
 * diskette numbers from 1 and a hard sectored one from 0, and a block
 * number is a position - the driver has to say which sector is at that
 * position, and saying zero when the track has no sector zero reads
 * nothing at all.  The kernel takes this from the drive's own
 * characteristics byte; see sys/dj.c and ORG1 in sys/dj.h.
 */
int cylstart INIT;
int roll INIT;
int heads INIT;
int spt INIT;
int spc INIT;
int limit INIT;
int firstsec INIT;

/*
 * reset the drive.  There is nothing to sense or probe: the drive is
 * the one the rom just read the boot from, so its constants are already
 * in the controller.  This prints the banner, writes the channel
 * program's invariant bytes, and reads the label.
 */
reset()
{
	register struct dlabel *lp;
	register int i;
	int sl;
	int got;

	outstr("Micronix loader for the DJ-DMA\n");

	chan[0] = SETDMA;
	chan[3] = 0;		/* dma high byte is always 0 */
	chan[4] = SREAD;
	chan[7] = 0;		/* drive 0 */
	chan[9] = HALT;

	/*
	 * The status bytes, so the first read's poll starts clean.  They
	 * are the last byte of each opcode - 0058 in the sread, 005a in
	 * the halt - so 0059 between them is the halt itself, and is not
	 * ours to clear.
	 */
	for (i = 0; i <= 1; i++)
		chan[8 + i * 2] = 0;

	/*
	 * The label, out of the block the rom read us from - the first
	 * sector of the first track, which is the one place on the medium
	 * that can be found knowing nothing about it.  It goes into boot.c's
	 * buf0 at 0x1000, which nothing has used yet.
	 *
	 * Its sector *number* is the one thing that cannot be known before
	 * it is read: a soft sectored medium numbers its sectors from 1 and
	 * a hard sectored one from 0, and the label is where that is written
	 * down.  So read both and keep the one carrying the magic - nothing
	 * else on that track is a label, so the magic is what says which
	 * base this medium uses, and the medium answers by failing to
	 * produce a sector it does not have.
	 */
	lp = (struct dlabel *)(0x1000 + DL_OFFSET);
	got = 0;
	for (i = 0; i < 2; i++) {
		if (!djrsec(0, 0, i, (char *)0x1000))
			continue;
		got = 1;
		if (lp->d_magic[0] == DL_MAGIC[0] &&
		    lp->d_magic[1] == DL_MAGIC[1] &&
		    lp->d_magic[2] == DL_MAGIC[2] &&
		    lp->d_magic[3] == DL_MAGIC[3])
			break;
	}
	if (i == 2) {
		if (!got)
			outstr("cannot read cylinder 0\n");
		/*
		 * There is nothing to fall back on, and that is deliberate.
		 * A disk that cannot say what it is cannot be read: every
		 * block number goes through the geometry, so guessing it
		 * means reading the wrong cylinder and calling whatever is
		 * there a filesystem.
		 */
		else {
			outstr("No disk label at cylinder 0.\n");
			outstr("The medium is read from there and guessed nowhere.\n");
			outstr("mnix bootflop writes one; mkbootimg builds one in.\n");
		}
		bail();
	}

	if (!lp->d_tracks || !lp->d_heads || !lp->d_spt) {
		outstr("The disk label has no geometry\n");
		bail();
	}

	/*
	 * Which sector carried it is the medium's answer and the label's
	 * is a claim; they have to be the same one, or every block number
	 * after this reads the wrong sector and calls what it finds there
	 * a filesystem.
	 */
	if (lp->d_firstsec != i) {
		outstr("Label and medium disagree about the first sector\n");
		bail();
	}
	firstsec = lp->d_firstsec;

	sl = lp->d_bootslice;
	if (sl >= NSLICE)
		sl = 0;
	cylstart = lp->d_slice[sl].d_off;
	roll = lp->d_roll;
	heads = lp->d_heads;
	spt = lp->d_spt;
	spc = heads * spt;
	/*
	 * A block number is a filesystem block number, so the count starts
	 * at the slice's first cylinder and not the disk's.  A slice with a
	 * length says where it ends; one without runs to the last cylinder
	 * of the medium.
	 */
	if (lp->d_slice[sl].d_len)
		limit = lp->d_slice[sl].d_len * spc - 1;
	else
		limit = (lp->d_tracks - cylstart) * spc - 1;
}

/*
 * Read one sector, from the cylinder the controller is told.
 *
 * The cylinder is an argument rather than derived from a block number
 * because the label has to be read before a block number means
 * anything, and the label is at cylinder 0.  sec is the sector's number
 * on the track as the medium numbers it, with the head in bit 7 - the
 * DJ-DMA's own encoding, and not a block number.
 */
static int
djrsec(cyl, head, sec, dma)
int cyl;
int head;
int sec;
char *dma;
{
	chan[1] = (UINT8)(UINT)dma;
	chan[2] = (UINT8)((UINT)dma >> 8);
	chan[5] = cyl;
	chan[6] = sec | (head << 7);

	chan[8] = 0;			/* say we are waiting for it */
	out(ATTN, 0);			/* run the channel */
	while (chan[8] == 0)		/* wait for the read's status */
		;
	return chan[8] == OKSTAT;
}

/*
 * One sector with retries, and loud about it.  The single attempt above
 * is silent because its caller asks for sectors a track may not have on
 * purpose - see the label probe in reset() - and a medium answering a
 * question with no is not a read error.
 */
int
djread(cyl, head, sec, dma)
int cyl;
int head;
int sec;
char *dma;
{
	tries = 0;
	while (tries++ < 10) {
		if (djrsec(cyl, head, sec, dma))
			return 1;
		outstr("retry\n");
	}
	outstr("Read error\n");
	return 0;
}

/*
 * given a block number, read the contents into the buffer.
 * return 1 for success.
 */
int
readblock(blocknum, buffer)
int blocknum;
char *buffer;
{
	register int cyl;
	int secnum;
	int head;

	/*
	 * boot.c works in filesystem block numbers: block 2 is the first
	 * inode block and the root directory is wherever its inode says.
	 * The same block-to-cylinder arithmetic as the kernel driver's
	 * sio(): cylinder is (blk / SPC) + cylstart, which is what keeps
	 * the boot's reserved cylinders out of the filesystem.
	 *
	 * A block number is a position in that layout and a sector number
	 * is a name on a track, and on a soft sectored medium the two do
	 * not agree: position 0 is sector 1.  djread wants the name.
	 */
	if (blocknum > limit) {
		outstr("Block out of range\n");
		return 0;
	}

	secnum = blocknum % spc;
	cyl = (blocknum / spc) + cylstart + roll;
	head = secnum / spt;

	return djread(cyl, head, secnum % spt + firstsec, buffer);
}
