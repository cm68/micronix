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
 * It comes off the medium instead, out of the label in the block the rom
 * read us from - the same struct dlabel a hard disk carries, saying the
 * same kind of thing.  The level 2 sector hands that block on rather than
 * this file reading it again: see reset() and djboot1.s.  See
 * sys/dlabel.h.
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
 * The label's magic - the four bytes that say a block holds a struct
 * dlabel at DL_OFFSET.  What it checks here is the handover: the rom
 * found a block 0 with this in it and the level 2 sector copied that
 * block whole, so anything else at 0x1000 means the copy did not happen
 * and the geometry below would be read out of whatever did.
 */
static int
hasmagic(lp)
register struct dlabel *lp;
{
	return lp->d_magic[0] == DL_MAGIC[0] && lp->d_magic[1] == DL_MAGIC[1] &&
	    lp->d_magic[2] == DL_MAGIC[2] && lp->d_magic[3] == DL_MAGIC[3];
}

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
int len INIT;		/* the boot slice's length, in cylinders */
int firstsec INIT;
int bps INIT;		/* blocks per sector: 2 on a 1K medium, else 1 */

/*
 * Where a 1K sector is staged while its half is copied out.  The DJ-DMA
 * reads a whole sector and a filesystem block on a 1K medium is half of
 * one, so the read goes here and the half to the caller's buffer.
 * boot.c points it at two consecutive holes in the kernel image, a
 * kilobyte the load does not write.  Until load() does that it is a fixed
 * address over the u page's bss, which nothing has loaded yet.
 */
extern char *sectbuf;

/*
 * reset the drive.  There is nothing to sense or probe: the drive is
 * the one the rom just read the boot from, so its constants are already
 * in the controller and its number is already in the channel program.
 * This prints the banner, writes the parts of that program which never
 * change, and takes the label from the block the level 2 sector copied
 * up to buf0.
 */
reset()
{
	register struct dlabel *lp;
	register int i;
	int sl;

	outstr("Micronix loader for the DJ-DMA\n");

	chan[0] = SETDMA;
	chan[3] = 0;		/* dma high byte is always 0 */
	chan[4] = SREAD;
	/*
	 * And the drive byte - chan[7] - is deliberately not written here,
	 * or anywhere in this directory.  It is the one the rom left there
	 * when it read the block this loader came from: the rom asks every
	 * drive on the controller for its block 0 and keeps the one whose
	 * diskette answers with a label, so the diskette read here is by
	 * construction the one in that drive, and the level 2 sector
	 * patches this same program without touching the byte either.  See
	 * stand/roms/l1boot.s's DJ row and djboot1.s.
	 */
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
	 * The label, out of the block the rom read.  djboot1.s copies the
	 * whole of that 512 byte block to boot.c's buf0 at 0x1000, which is
	 * where this would have read it to anyway and which nothing has
	 * used yet, so there is nothing to read here.
	 *
	 * That is the medium's doing and not a shortcut.  Which sector of a
	 * block 0 carries the label depends on what a block 0 is, which
	 * depends on the drive: on an eight inch drive, cylinder 0 head 0 is
	 * single density - fm, 26 sectors of 128 bytes numbered from one -
	 * so block 0 is the first four of them and the label is four
	 * sectors in, while a five inch drive's block 0 is one whole sector
	 * whose number the medium only states by not answering to the other
	 * one.  The rom asked both questions to get here: it reads block 0
	 * off every drive on the controller and keeps the one that answers
	 * with a label, which is the same thing as knowing the numbering and
	 * the shape of the block.  What it read is this block, and the level
	 * 2 sector kept it whole.  See stand/roms/l1boot.s's djblock and
	 * djboot1.s.
	 */
	lp = (struct dlabel *)(0x1000 + DL_OFFSET);
	if (!hasmagic(lp))
		bail("No disk label in the block this boot came from\n");

	/*
	 * Where the medium numbers its sectors from is the label's to state
	 * and the rom's to have settled: every block number below is turned
	 * into a sector name with it, and a name the track does not have
	 * reads nothing at all.  The rom settled it by asking for the sector
	 * that carries the magic.  It is also the first sector of a track,
	 * which is where level 2 put level 3.
	 */
	firstsec = lp->d_firstsec;

	sl = lp->d_bootslice;
	if (sl >= NSLICE)
		sl = 0;
	cylstart = lp->d_slice[sl].d_off;
	roll = lp->d_slice[sl].d_roll;
	len = lp->d_slice[sl].d_len;
	heads = lp->d_heads;
	spt = lp->d_spt;

	/*
	 * How many blocks a sector is.  The label's d_spt counts 512 byte
	 * blocks per track, not physical sectors - which is how a 1K
	 * medium's five sectors read as ten - so the sector size is
	 * inferred from that count and from how the medium numbers its
	 * sectors.  The DJ-DMA's soft sectored media number from one and
	 * put 1024 bytes in a sector, five to a track on five inch (ten
	 * blocks) and eight on eight inch (sixteen); a hard sectored five
	 * inch numbers from zero and is ten 512 byte sectors, and the eight
	 * inch 512 medium is fifteen.  So a soft sectored medium with ten
	 * or sixteen blocks to a track is 1K - two blocks to a sector - and
	 * every other count this loader can meet is 512.  A medium of 26
	 * blocks is 128 byte sectors and is not a filesystem medium; this
	 * would misread it, so it is refused rather than guessed.
	 */
	if (firstsec == 1 && (spt == 10 || spt == 16))
		bps = 2;
	else if (spt == 9 || spt == 10 || spt == 15)
		bps = 1;
	else
		bail("The disk label has a sector count this loader does not know\n");
	spc = heads * spt;	/* blocks per cylinder */
	/*
	 * A block number is a filesystem block number, so the count starts
	 * at the slice's first cylinder and not the disk's.  A slice that
	 * has not been carved (length zero) cannot boot.
	 */
	if (!len)
		bail("Boot slice is not carved.\n");
	limit = len * spc - 1;
}

/*
 * Read one sector, from the cylinder it is handed.
 *
 * This is the primitive a block read is built on and it takes the
 * medium's own coordinates - a cylinder, a head and a sector's number on
 * the track, with the head in bit 7 of that last one, which is the
 * DJ-DMA's own encoding and not a block number.  Turning a filesystem
 * block number into those is readblock()'s, below.
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
 * is the silent one: it answers whether the sector came back or not, and
 * everything that has something to say about a failure says it here.
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
 * One block out of a 1K sector.  The controller reads a whole sector,
 * so a block - half a sector - is the full read into SECTBUF and the
 * half copied out.  half is which 512 bytes: 0 the first, 1 the second.
 */
static int
djrblk(cyl, head, sec, half, dma)
int cyl;
int head;
int sec;
int half;
char *dma;
{
	register char *s;
	register char *d;
	register int n;

	if (!djread(cyl, head, sec, sectbuf))
		return 0;
	s = sectbuf + (half ? 512 : 0);
	d = dma;
	for (n = 512; n; n--)
		*d++ = *s++;
	return 1;
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
	register int track;
	register int within;
	register int cyl;
	int secnum;
	int half;
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
	 *
	 * d_spt is blocks per track, so the track and the position in it
	 * come straight from it; a block is one whole sector on a 512 byte
	 * medium and half of one on a 1K medium, so the position is divided
	 * by bps to name the sector and its remainder is which half.
	 */
	track = blocknum / spt;
	within = blocknum % spt;
	cyl = (track / heads) + roll;
	if (cyl >= len)
		cyl -= len;
	cyl += cylstart;
	head = track % heads;
	secnum = within / bps + firstsec;
	half = within % bps;

	if (bps == 1)
		return djread(cyl, head, secnum, buffer);
	return djrblk(cyl, head, secnum, half, buffer);
}
