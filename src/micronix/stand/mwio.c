/*
 * this file is the hdc-dma boot code for micronix.
 */
#include <types.h>
#include <sys/mw.h>
#include <sys/dlabel.h>

struct drivespec {
	UINT cylinders;
	UINT8 heads;
	UINT8 spt;			/* sectors per track */
	UINT limit;			/* max block number */
	UINT8 spc;			/* sectors per track */
	UINT roll;			/* what mw.c adds to blk / spc */
} spec = {
	153, 4, 17, (153 * 4 * 17) -1, 4 * 17, 153 / 2	/* st506 */
};

#define	STEPDELAY	30
#define	SETTLE		100

/*
 * Fixed addresses, reached through pointers rather than as
 *
 *	*(int *)HDC_CCA = &cmd;
 *
 * because the compiler drops a store through a cast constant and says
 * nothing - no error, no exit status, just an object file with the
 * statement missing.  That is what left the controller pointing at the
 * first level's command block, re-executing it and never writing a
 * status byte, while the loader spun on it.  See ccc/CASTBUG.
 *
 * Put back when the compiler takes the shape; the initialisers below
 * are the same constants and nothing else has to change.
 */
UINT *ccaptr = (UINT *)HDC_CCA;         /* channel command address */
UINT8 *ccahigh = (UINT8 *)(HDC_CCA + 2);
UINT8 *hdrbuf = (UINT8 *)0x84;          /* where read header lands a head */

/*
 * whitesmith's stupidity means that BSS symbols don't link unless they
 * have an explicit initializer.
 */
#ifndef __STDC__
#define	INIT	= 0
#endif

extern char disk0buf[];		/* boot.c's, and not yet in use */

char tries INIT;
int curcyl INIT;

struct hddma_cmd cmd = { 0 };   /* braces: a struct is sized from its type */

/*
 * reset the drive.
 * we do something a bit clever here:
 * assume a slow drive.  assume 17 sectors/track.
 * do a read header on each of the 8 possible heads to
 * probe the possible heads.
 */
reset()
{
	register struct dlabel *lp;

    outstr("Micronix loader for the HD-DMA\n");

	/*
	 * Tell the controller where its command block is.  This wrote the
	 * pointer to 5d, and the controller reads it from 50 - it is the
	 * default channel command address, the same one BOOTMW's equates
	 * name and the same one the rom's boot uses.  So the controller
	 * went on executing whatever the rom had left at 50, never wrote a
	 * status byte, and hdc_command spun on it forever.  The banner
	 * printed and nothing else did.
	 */
	*ccaptr = (UINT)&cmd;
	*ccahigh = 0;

    out(HDC_RESET, 0);

	cmd.steps = 0;
    cmd.byte0 = 0;
    cmd.byte1 = STEPDELAY;
    cmd.byte2 = SETTLE;
    cmd.byte3 = SEC512;
	cmd.link = &cmd;
	cmd.xlink = 0;

    hdc_command(OP_LOAD);

    cmd.drvsel = 0;
    cmd.headsel = 0;
    hdc_command(OP_SENSE);

    if (cmd.status & SENSE_READY) {
		outstr("Drive not ready\n");
		bail();		/* not exit(): stdio comes in behind it */
	}

	/* recalibrate */
	cmd.steps = 0xffff;
    cmd.drvsel |= STEPOUT;
    hdc_command(OP_NOP);

	/*
	 * The geometry, out of the label in the block we were loaded
	 * from.  mkfs writes it there when it installs a boot, and
	 * stand/mkbootimg builds one image per drive with it already in -
	 * so the answer is on the disk and does not have to be guessed.
	 *
	 * This used to probe: read header on each of the eight possible
	 * heads and see which answered.  The command takes no head
	 * argument and answers with whatever comes round next, so what
	 * came back was byte 0 of a header the drive chose, the probe
	 * matched only head 0 and gave up at 1, and every block number
	 * after that landed on the wrong cylinder.
	 *
	 * Physical cylinder 0, head 0, sector 0 needs no geometry to
	 * reach, which is what makes this possible before the geometry is
	 * known.  disk0buf is boot.c's and nothing has been read into it
	 * yet.
	 */
	cmd.steps = curcyl;
	cmd.drvsel = STEPOUT;
	curcyl = 0;
	cmd.word0 = 0;			/* cylinder */
	cmd.byte2 = 0;			/* head */
	cmd.byte3 = 0;			/* sector */
	cmd.headsel = (~0 & 7) << 2;
	cmd.dma = (UINT)disk0buf;
	cmd.xdma = 0;

	tries = 0;
	while (tries++ < 10) {
		if (hdc_command(OP_READ))
			break;
	}
	if (tries > 10)
		outstr("label read failed\n");

	lp = (struct dlabel *)&disk0buf[DL_OFFSET];
	if (lp->d_magic[0] == DL_MAGIC[0] && lp->d_magic[1] == DL_MAGIC[1] &&
	    lp->d_magic[2] == DL_MAGIC[2] && lp->d_magic[3] == DL_MAGIC[3] &&
	    lp->d_tracks && lp->d_heads && lp->d_spt) {
		spec.cylinders = lp->d_tracks;
		spec.heads = lp->d_heads;
		spec.spt = lp->d_spt;
		spec.spc = lp->d_heads * lp->d_spt;
		spec.roll = lp->d_roll;
		spec.limit = lp->d_tracks * spec.spc - 1;
	} else {
		/*
		 * No label, so the table's geometry stands - it is what we
		 * booted from.  Say so: a disk that reads but cannot
		 * describe itself is worth knowing about before the block
		 * numbers start.
		 */
		outstr("No disk label, assuming st506\n");
	}

}

/*
 * given a block number, read the contents into the buffer
 * return 1 for success
 */
int
#ifndef __STDC__
readblock(blocknum, buffer)
int blocknum;
char *buffer;
#else
readblock(int blocknum, char *buffer)
#endif
{
	register int cyl;
	int secnum;
	int trknum;
	int head;

	if (blocknum > spec.limit) {
        outstr("Block out of range\n");
		return 0;
    }

	/*
	 * Where a block lives, the way sys/mw.c puts it there.
	 *
	 * spc is sectors per cylinder - 68 on an st506 - whatever the
	 * comment on it says, so blocknum / spc is already the cylinder
	 * and dividing that by heads again was wrong, as was taking the
	 * head from it.  The sector came out right by accident, because
	 * secnum % spt is the same either way.
	 *
	 * And the rotation, which was missing altogether.  mw.c adds half
	 * the cylinder count and wraps, so that the superblock and the
	 * inodes sit in the middle of the platter and the average seek to
	 * them halves.  Without it this looked for the superblock at
	 * cylinder 0 when it lives at cylinder 76, and read whatever was
	 * there - which is the boot area, and so never got a filesystem
	 * at all.
	 */
	secnum = blocknum % spec.spc;
	cyl = blocknum / spec.spc + spec.roll;
	if (cyl >= spec.cylinders)
		cyl -= spec.cylinders;
	head = secnum / spec.spt;

	if (curcyl < cyl) {
		cmd.steps = cyl - curcyl;
		cmd.drvsel = 0;
	} else {
		cmd.steps = curcyl - cyl;
		cmd.drvsel = STEPOUT;
	}
	curcyl = cyl;
	cmd.word0 = cyl;
	cmd.byte2 = head;
	cmd.byte3 = secnum % spec.spt;
    cmd.headsel = (~cmd.byte2 & 7) << 2;
	cmd.dma = (UINT)buffer;
	cmd.xdma = 0;

	tries = 0;
	while (tries++ < 10) {
		if (hdc_command(OP_READ)) {
			return 1;
		}
        outstr("retry\n");
    }
    outstr("Read error\n");
    return 0;
}

/*
 * run a hdc command and wait for a response
 */
int 
#ifdef __STDC__
hdc_command(UINT8 opcode)
#else
hdc_command(opcode)
UINT8 opcode;
#endif
{
    cmd.opcode = opcode;
    cmd.status = 0;
    out(HDC_ATTN, 0);
	while (cmd.status == 0)
		;
    if (cmd.status == 0xff) {
		return 1;
	} else {
		return 0;
	}
}
