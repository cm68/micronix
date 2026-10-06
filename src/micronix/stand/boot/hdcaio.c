/*
 * this file is the hdca boot code for micronix.
 *
 * The second level of the hard disk boot for a drive on the HDCA - the
 * port-mapped, non-DMA Winchester controller of the M10, M20 and M26.
 * It is the same protocol as sys/hdca.c, and the same card, and the two
 * are meant to be read together; what differs is that this one runs
 * before the kernel, with no buffer cache and no interrupts, and reads
 * one caller's buffer at a time, polling the status registers instead
 * of arming a continuation.
 *
 * The shape is mwio.c's, because the filesystem is the same one: reset
 * reads the disk label out of the boot cylinder, the geometry comes from
 * there and is guessed nowhere, and the block mapping is the same
 * rotation the kernel uses.  What is missing is everything the HD-DMA
 * needs and this card does not - no channel command block, no sense or
 * recalibrate or load-constants, and no status byte to poll in memory,
 * because the four registers carry it all.
 */

#include <types.h>
#include <sys/dlabel.h>
#include <sys/hdca.h>

struct drivespec {
	UINT cylinders;
	UINT8 heads;
	UINT8 spt;			/* sectors per track */
	UINT limit;			/* max block number */
	UINT spc;			/* sectors per cylinder */
	UINT roll;			/* what the cylinder is rotated by */
	UINT cylstart;			/* the slice's first cylinder */
	UINT8 firstsec;			/* first sector number on a track */
} spec = {
	/*
	 * Nothing.  The geometry comes off the disk and there is no other
	 * source for it, the same reason as the other drivers: a compiled-
	 * in table is a second opinion about a drive this program has in
	 * front of it.  An initialiser of zeroes because Whitesmith's will
	 * not link a bss symbol without one.
	 */
	0, 0, 0, 0, 0, 0, 0, 0
};

#define	TRIES	10		/* per sector, as the other drivers retry */

/*
 * whitesmith's stupidity means that BSS symbols don't link unless they
 * have an explicit initializer.
 */
#define	INIT	= 0

char tries INIT;
int curcyl INIT;

/*
 * Select drive 0 and a head, and put the controller in its working
 * state: front-end enabled, running, on the drive clock, writes on.
 */
static void
hdselect(head)
	UINT8 head;
{
	out(HDCA_CONTROL, CONT_FRENBL | CONT_RUN | CONT_DSKCLK | CONT_WPROT);
	out(HDCA_FUNC, (0 & FUNC_DRIVE) | FUNC_STEP | FUNC_DIR | (((head ^ 0x0f) << 4) & FUNC_HEAD));
}

/*
 * Step the arm n tracks - dir 0 is up, 1 is down - and wait for the
 * controller to say the seek is done.  The step line is pulsed
 * high-low-high per track, and the seek-done bit is acknowledged by
 * reading the interrupt register, the way sys/hdca.c's hdint does.
 */
static void
hdstep(head, dir, n)
	UINT8 head, dir;
	UINT n;
{
	UINT8 fn;
	UINT i;

	fn = (0 & FUNC_DRIVE) | FUNC_STEP | (((head ^ 0x0f) << 4) & FUNC_HEAD);
	if (dir)
		fn |= FUNC_DIR;
	for (i = 0; i < n; i++) {
		out(HDCA_FUNC, fn);		/* step line high */
		out(HDCA_FUNC, fn & ~FUNC_STEP);/* and low */
		out(HDCA_FUNC, fn);		/* rising edge: one track */
	}
	while (!(in(HDCA_SSR) & SSR_SDONE))
		;
	(void)in(HDCA_INT);	/* acknowledge seek-done */
	out(HDCA_CMD, CMD_RAMDATA);
}

/*
 * Read one sector into p.  Returns 1, or 0 if the controller reported a
 * fault.
 *
 * The header (head, track, sector, key) is written to the controller's
 * header RAM, the read is issued, the op-done bit is polled, and the
 * sector is pulled through the data port.  A read stores the sector at
 * offset +2 with its last two bytes wrapped to the front, so two slack
 * bytes are read and discarded first and the 512 that follow are the
 * sector in order.
 */
static int
hdread(head, track, sector, p)
	UINT8 head, track, sector;
	char *p;
{
	UINT i;
	UINT8 psr, ssr;

	out(HDCA_CMD, CMD_RAMHDR);
	out(HDCA_DATA, head);
	out(HDCA_DATA, track);
	out(HDCA_DATA, sector);
	out(HDCA_DATA, 0);	/* key */

	out(HDCA_CMD, CMD_READSEC);
	while (!(in(HDCA_PSR) & PSR_OPDONE))
		;

	psr = in(HDCA_PSR);
	ssr = in(HDCA_SSR);
	if ((psr & PSR_TIMOUT) || (ssr & SSR_DATACRC) || !(psr & PSR_NFAULT))
		return 0;

	out(HDCA_CMD, CMD_RAMDATA);
	(void)in(HDCA_DATA);	/* two slack bytes */
	(void)in(HDCA_DATA);
	for (i = 0; i < 512; i++)
		p[i] = in(HDCA_DATA);
	return 1;
}

/*
 * reset the drive, and read the label that says how the disk is laid
 * out.
 *
 * Unlike mwio.c there is almost nothing to prepare: an HD-DMA holds
 * state the host put there - a command block, a selected drive, a set of
 * constants - and the HDCA holds its own.  The controller is enabled and
 * drive 0 selected, and then the one read everything else depends on.
 */
reset()
{
	register struct dlabel *lp;
	int sl;

	outstr("Micronix loader for the HDCA\n");

	hdselect(0);

	/*
	 * The geometry, out of the label in the boot cylinder.  mkfs
	 * writes it there when it installs a boot, and takes it from the
	 * drive it is installing onto - so the answer is on the disk and
	 * does not have to be guessed.
	 *
	 * Block 0 - physical cylinder 0, head 0 - needs no geometry to
	 * reach, which is what makes this possible before the geometry is
	 * known.  The HDCA's formatter numbers the first sector on a track
	 * 1, so block 0 is sector 1, not 0 (FORMATHD's skew table runs
	 * 1..32).  It is the sector the rom read this loader's first level
	 * from, it is where mkfs writes the label, and it lands in the
	 * working buffer, boot.c's buf0 at 0x1000, which nothing has used
	 * yet.
	 */
	tries = 0;
	while (tries++ < TRIES) {
		if (hdread(0, 0, 1, (char *)0x1000))
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
		spec.firstsec = lp->d_firstsec;

		/*
		 * And the slice the boot is in, which on a rolled disk is
		 * the empty one: d_slice[0].d_off is 0, so cylstart is 0 and
		 * the two lines below are exactly what they were before there
		 * were tables.  See sys/dlabel.h.
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
		 * block number goes through the geometry, so guessing it means
		 * reading the wrong cylinder and calling whatever is there a
		 * filesystem.
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
	UINT8 head, sector;

	if (blocknum > spec.limit) {
		outstr("Block out of range\n");
		return 0;
	}

	/*
	 * Where a block lives, the way sys/hdca.c puts it there.
	 *
	 * spc is sectors per cylinder, so blocknum / spc is already the
	 * cylinder; the rotation and the slice's start are added and the
	 * result wrapped at the cylinder count.  The head and sector are
	 * what is left within the cylinder.  spc is a UINT, so blocknum is
	 * converted to unsigned before the division: a large volume's last
	 * block numbers do not fit in a signed int.
	 */
	secnum = blocknum % spec.spc;
	cyl = blocknum / spec.spc + spec.roll + spec.cylstart;
	if (cyl >= spec.cylinders)
		cyl -= spec.cylinders;
	head = secnum / spec.spt;
	sector = (blocknum % spec.spt) + spec.firstsec;

	if (curcyl != (int)cyl) {
		hdselect(head);
		if (curcyl < (int)cyl)
			hdstep(head, 0, cyl - curcyl);
		else
			hdstep(head, 1, curcyl - cyl);
		curcyl = cyl;
	}

	tries = 0;
	while (tries++ < TRIES) {
		if (hdread(head, (UINT8)cyl, sector, buffer))
			return 1;
		outstr("retry\n");
	}
	outstr("Read error\n");
	return 0;
}

/* vim: tabstop=4 shiftwidth=4 noexpandtab: */
