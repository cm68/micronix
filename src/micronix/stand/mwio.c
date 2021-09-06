/*
 * this file is the hdc-dma boot code for micronix.
 */
#include <types.h>
#include <sys/mw.h>

struct drivespec {
	UINT cylinders;
	UINT8 heads;
	UINT8 spt;			/* sectors per track */
	UINT limit;			/* max block number */
	UINT8 spc;			/* sectors per track */
} spec = {
	153, 4, 17, (153 * 4 * 17) -1, 4 * 17		/* st506 */
};

#define	STEPDELAY	30
#define	SETTLE		100

/*
 * whitesmith's stupidity means that BSS symbols don't link unless they
 * have an explicit initializer.
 */
#ifndef __STDC__
#define	INIT	= 0
#endif

char tries INIT;
int curcyl INIT;

struct hddma_cmd cmd INIT;

/*
 * reset the drive.
 * we do something a bit clever here:
 * assume a slow drive.  assume 17 sectors/track.
 * do a read header on each of the 8 possible heads to
 * probe the possible heads.
 */
reset()
{
	register int i;

    outstr("Micronix loader for the HD-DMA\n");

	*(int *)0x5d = &cmd;
	*(char *)0x5f = 0;

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
		exit();
	}

	/* recalibrate */
	cmd.steps = 0xffff;
    cmd.drvsel |= STEPOUT;
    hdc_command(OP_NOP);

	/* let's see if we can find out how many heads we have */
	for (i = 0; i < 8; i++) {
		cmd.headsel = (~i & 7) << 2;
		cmd.dma = (UINT)0x80;
		cmd.xdma = 0;
		tries = 0;
		while (1) {
			*(char *)0x84 = 0xff;
			if (!hdc_command(OP_HEADER)) {
				if (tries++ > 10) goto probedone;
			}
			if (*(char *)0x84 == i) {
				break;
			}
		}
		outstr("head\n");
	}
probedone:
	spec.heads = i;
	spec.spc = i * spec.spt;
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

	secnum = blocknum % spec.spc;
	trknum = blocknum / spec.spc;
	cyl = trknum / spec.heads;
	head = trknum % spec.heads;

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
