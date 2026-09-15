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
 * The geometry is fixed: a five inch floppy is 40 cylinders, two heads,
 * ten 512 byte sectors to a track.  The first 40 blocks are the system
 * tracks - the boot the rom reads - so the filesystem's block 1, the
 * superblock, is block 40 on the disk, and the block numbers boot.c
 * hands readblock are 39 short of the disk's.
 */
#include <types.h>

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

/*
 * The five inch floppy, one row of the DJ-DMA firmware format table -
 * the same fields dj.c's specs[] carries.  40 cylinders, two heads, ten
 * sectors to a track, and two cylinders reserved for the boot, which is
 * the toff (track offset) the kernel driver adds when it maps a block
 * to a cylinder.  Sectors are numbered from 0.
 */
#define	CYLINDERS 40
#define	HEADS	2
#define	SPT	10
#define	SPC	(HEADS * SPT)	/* sectors per cylinder */
#define	TOFF	2		/* reserved cylinders, the boot tracks */
#define	LIMIT	((CYLINDERS - TOFF) * SPC - 1)

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

char tries INIT;
int curcyl INIT;

/*
 * reset the drive.  There is nothing to sense or probe: the geometry
 * is fixed, and the drive is the one the rom just read the boot from,
 * so its constants are already in the controller.  This only prints
 * the banner and writes the channel program's invariant bytes.
 */
reset()
{
	register int i;

	outstr("Micronix loader for the DJ-DMA\n");

	chan[0] = SETDMA;
	chan[3] = 0;		/* dma high byte is always 0 */
	chan[4] = SREAD;
	chan[7] = 0;		/* drive 0 */
	chan[9] = HALT;

	/* the status bytes, so the first read's poll starts clean */
	for (i = 0; i <= 1; i++)
		chan[8 + i] = 0;
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
	 * boot.c works in filesystem block numbers: block 0 is the boot
	 * block, block 1 the superblock, block 2 the first inode block.
	 * The same block-to-cylinder arithmetic as the kernel driver's
	 * sio(): cylinder is (blk / SPC) + TOFF, which is what keeps the
	 * boot's reserved cylinders out of the filesystem.
	 */
	if (blocknum > LIMIT) {
		outstr("Block out of range\n");
		return 0;
	}

	secnum = blocknum % SPC;
	cyl = (blocknum / SPC) + TOFF;
	head = secnum / SPT;

	/* the channel program's per-read bytes: dma address and sector */
	chan[1] = (UINT8)(UINT)buffer;
	chan[2] = (UINT8)((UINT)buffer >> 8);
	chan[5] = cyl;
	chan[6] = (secnum % SPT) | (head << 7);

	tries = 0;
	while (tries++ < 10) {
		chan[8] = 0;		/* say we are waiting for it */
		out(ATTN, 0);		/* run the channel */
		while (chan[8] == 0)	/* wait for the read's status */
			;
		if (chan[8] == OKSTAT)
			return 1;
		outstr("retry\n");
	}
	outstr("Read error\n");
	return 0;
}
