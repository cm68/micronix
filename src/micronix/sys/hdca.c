/*
 * The HDCA disk driver.  sys/hdca.c
 *
 * The port-mapped, non-DMA Winchester controller of the M10, M20 and
 * M26, reconstructed from the hd driver in the 1.4 kernel and the
 * controller emulation in hwsim/d1/hdca.c.  The function names follow
 * the 1.4 driver: hdopen/hdclose/hdstrat/hdint, the helpers
 * hdrwse/hdproc/hdtrk/hdrhdr/hdup/hdstep/hdrdat/hdwdat, the geometry
 * getinfo/gettrk/getsec/gethed, and the phase state machine phase00
 * through phase12.
 *
 * Like the 1.4 driver, the driver never spins on the controller.  It
 * arms a continuation - the phase to resume - issues the command, and
 * returns; the op-done interrupt resumes the armed phase and the
 * seek-done interrupt resumes the armed seek phase.  The data is moved
 * and the buffer finished (iodone) inside the interrupt handler, the
 * way the original did and the way sys/dj.c still does.  The controller
 * is shared by its four drives, so one request runs at a time and the
 * rest queue on their drive.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/dlabel.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/hdca.h>
#include <sys/ovl.h>
#include <errno.h>

extern int in(), out(), timeout(), inton();

struct hd hdinfo[NHD];

static int hdmutex;		/* the controller, 0 = free */
static int (*hdintr) ();	/* the phase the op-done interrupt resumes */
static struct hd *hdintarg;	/* its argument */
static int (*hdseekintr) ();	/* the phase the seek-done interrupt resumes */
static struct hd *hdseekarg;	/* its argument */
static UINT8 hdunit;		/* the drive the FUNC register selects */
static struct hd *hdactive;	/* the drive owning the controller */
static UINT8 hddead;		/* seconds the request has been in flight */
static int hdioctl;		/* the ioctl sleep channel */

static void phase00(), phase01(), phase02(), phase03(), phase04(),
    phase06(), phase08(), phase09(), phase11(), phase12();
static struct hd *getinfo();

/*
 * Put the drive and head into the FUNC register.  The head is inverted
 * on the bus, and the drive's own bits ride in the low two.
 */
static void
hdselect(drive, head)
	UINT8 drive;
	UINT8 head;
{
	out(HDCA_CONTROL, CONT_FRENBL | CONT_RUN | CONT_DSKCLK | CONT_WPROT);
	out(HDCA_FUNC, (drive & FUNC_DRIVE) | FUNC_STEP | FUNC_DIR |
	    (((head ^ 0x0f) << 4) & FUNC_HEAD));
	hdunit = drive;
}

/*
 * Step the arm n tracks, up or down, then restart the RAM pointer.  The
 * step line is pulsed high-low-high per track; the controller moves one
 * track on the rising edge after a fall (hwsim/d1/hdca.c) and raises the
 * seek-done interrupt when the arm settles.
 */
static void
hdstep(head, dir, n)
	UINT8 head;
	UINT8 dir;
	UINT8 n;
{
	UINT8 fn;
	UINT8 i;

	fn = (hdunit & FUNC_DRIVE) | FUNC_STEP | (((head ^ 0x0f) << 4) & FUNC_HEAD);
	if (dir)
		fn |= FUNC_DIR;
	for (i = 0; i < n; i++) {
		out(HDCA_FUNC, fn);		/* step line high */
		out(HDCA_FUNC, fn & ~FUNC_STEP);/* and low */
		out(HDCA_FUNC, fn);		/* rising edge: one track */
	}
	out(HDCA_CMD, CMD_RAMDATA);
}

/*
 * Read one byte from the controller RAM at the current pointer.
 */
static UINT8
hdin()
{
	return (in(HDCA_DATA));
}

/*
 * Write one byte to the controller RAM at the current pointer.
 */
static void
hdout(b)
	UINT8 b;
{
	out(HDCA_DATA, b);
}

/*
 * Ask the controller how the last operation went.  Non-zero means the
 * operation failed.
 */
static UINT8
hdproc()
{
	UINT8 psr, ssr, err;

	psr = in(HDCA_PSR);
	ssr = in(HDCA_SSR);
	err = 0;
	if (psr & PSR_TIMOUT)
		err |= 0x02;
	if (ssr & SSR_DATACRC)
		err |= 0x04;
	if (!(psr & PSR_NFAULT))
		err |= 0x08;
	return (err);
}

/*
 * Point the controller at its header area and write the four header
 * bytes the next read/write command keys on: head, track, sector, key.
 */
static void
hdhdr(head, track, sector)
	UINT8 head, track, sector;
{
	out(HDCA_CMD, CMD_RAMHDR);
	hdout(head);
	hdout(track);
	hdout(sector);
	hdout(0);
}

/*
 * Read the track number the last header command returned.  The header
 * area's four bytes are head, track, sector, key; the track is the
 * second.
 */
static UINT
hdtrk()
{
	UINT v;

	out(HDCA_CMD, CMD_RAMHDR);
	(void)hdin();		/* head */
	v = hdin();		/* track */
	return (v);
}

/*
 * Read the four header bytes the last header command returned: head,
 * track, sector, key - the same four hdhdr() writes.
 */
static void
hdhdrrd(p)
	char *p;
{
	out(HDCA_CMD, CMD_RAMHDR);
	p[0] = hdin();
	p[1] = hdin();
	p[2] = hdin();
	p[3] = hdin();
}

/*
 * Write the four header bytes a header write keys on, verbatim.
 */
static void
hdhdrwr(p)
	char *p;
{
	out(HDCA_CMD, CMD_RAMHDR);
	hdout(p[0]);
	hdout(p[1]);
	hdout(p[2]);
	hdout(p[3]);
}

/*
 * Move a sector between the controller and the buffer.  A read leaves
 * the controller's RAM pointer two bytes in - the sector is stored at
 * offset +2 with its last two bytes wrapped to the front - so two slack
 * bytes are read and discarded first, then the 512 that follow are the
 * sector in order.  A write is plain.  bhold() has mapped the buffer's
 * segment and handed back its data address.
 */
static void
hdrdat(p, n)
	char *p;
	UINT n;
{
	UINT i;

	out(HDCA_CMD, CMD_RAMDATA);
	(void)hdin();
	(void)hdin();
	for (i = 0; i < n; i++)
		p[i] = hdin();
}

static void
hdwdat(p, n)
	char *p;
	UINT n;
{
	UINT i;

	out(HDCA_CMD, CMD_RAMDATA);
	for (i = 0; i < n; i++)
		hdout(p[i]);
}

/*
 * The per-drive state, by drive number (the 1.4 getinfo).
 */
static struct hd *
getinfo(dev)
	UINT dev;
{
	return (&hdinfo[dev & 3]);
}

/*
 * The block-to-geometry helpers.  A block becomes a cylinder, a head and
 * a sector the way every other disk on the machine does: divide by the
 * sectors per cylinder, apply the slice's start and the roll, then the
 * head is what is left divided by the sectors per track and the sector
 * is the remainder.
 */
static UINT
gettrk(blk, hp)
	UINT blk;
	struct hd *hp;
{
	UINT cyl;

	cyl = blk / hp->h_spc;
	cyl += hp->h_cylstart;
	cyl += hp->h_roll;
	cyl %= hp->h_tracks;
	return (cyl);
}

static UINT8
gethed(blk, hp)
	UINT blk;
	struct hd *hp;
{
	return ((blk % hp->h_spc) / hp->h_sectors);
}

static UINT8
getsec(blk, hp)
	UINT blk;
	struct hd *hp;
{
	return ((blk % hp->h_sectors) + hp->h_firstsec);
}

/*
 * Drive arbitration.  The controller serves one drive at a time; a
 * request that finds it busy is queued on its drive's h_next.  phase00()
 * below is called with the controller free.
 */
static int
exclude(hp)
	struct hd *hp;
{
	if (!hdmutex) {
		hdmutex = 1;
		return (0);
	}
	hp->h_flags |= HDWAIT;
	hp->h_next = hp->h_buf;
	return (-1);
}

/*
 * Hand the controller to the next queued request, or free it.
 */
static void
release(hp)
	struct hd *hp;
{
	struct hd *q;
	UINT i;

	for (i = 0; i < NHD; i++) {
		q = &hdinfo[i];
		if (q->h_next) {
			q->h_buf = q->h_next;
			q->h_next = 0;
			q->h_flags &= ~HDWAIT;
			phase00(q);
			return;
		}
	}
	hdmutex = 0;
	hdactive = 0;
}

/*
 * The watchdog.  Named as the tick in the header (sys/hdcahdr.c), so the
 * resident side (sys/ovl.c) calls this once a second rather than it being
 * a timeout() of its own - a driver-owned timer that never stops would
 * hold one of the kernel's five timeout slots for the rest of the boot.
 * This is mw.c's mwcheck, in the same shape.
 *
 * A controller that has stopped answering raises no interrupt, which is
 * the whole reason this exists: a request whose seek or read/write never
 * completes sits here past HDDEADLINE seconds and is failed with EIO, the
 * only way it ever ends.  Normal requests finish in milliseconds, so the
 * counter only reaches the deadline when the drive is wedged.
 */
int
hdtick()
{
	if (!hdmutex)
		return (0);
	if (++hddead <= HDDEADLINE)
		return (0);
	/*
	 * The armed continuations are dropped so a late interrupt - if the
	 * controller ever does answer - finds nothing to resume.
	 */
	di();
	hdseekintr = 0;
	hdseekarg = 0;
	hdintr = 0;
	hdintarg = 0;
	ei();
	if (hdactive)
		phase11(hdactive);
	return (0);
}

/*
 * The strategy entry: a block I/O from the buffer cache.  The device
 * number's low bits are the drive.  The request is queued, and if the
 * controller is free the arm starts moving toward it; the interrupts
 * carry it the rest of the way.
 */
int
hdstrat(b)
	struct buf *b;
{
	struct hd *hp;
	UINT cyl, head, sector;

	if (b->count != 512) {
		b->flags |= BERROR;
		b->error = EIO;
		iodone(b);
		return (0);
	}
	hp = getinfo(b->dev);
	if (b->blk > hp->h_maxblk) {
		b->flags |= BERROR;
		b->error = ENXIO;
		iodone(b);
		return (0);
	}
	cyl = gettrk(b->blk, hp);
	head = gethed(b->blk, hp);
	sector = getsec(b->blk, hp);
	hp->h_buf = b;
	hp->h_cur = b;
	hp->h_head = head;
	hp->h_sector = sector;
	hp->h_trackdelta = cyl - hp->h_cyl;
	if (b->flags & BREAD)
		hp->h_flags &= ~HDWRITE;
	else
		hp->h_flags |= HDWRITE;

	if (exclude(hp) < 0)
		return (0);
	phase00(hp);
	return (0);
}

/*
 * Begin a request: the arm's target is the request's cylinder, and the
 * seek (or, if it is already there, the transfer) starts from here.
 */
static void
phase00(hp)
	struct hd *hp;
{
	hp->h_retry = 0;
	hp->h_flags &= ~HDERROR;
	hdactive = hp;
	hddead = 0;
	phase01(hp);
}

/*
 * The seek.  If the arm is not at the target cylinder, step the whole
 * remaining distance and let the seek-done interrupt finish the move;
 * otherwise go straight to the transfer.
 */
static void
phase01(hp)
	struct hd *hp;
{
	if (hp->h_trackdelta == 0) {
		if (hp->h_cmd == HDCA_HOME) {
			hp->h_stat = 0;
			phase12(hp);
			return;
		}
		if (hp->h_flags & HDWRITE)
			phase08(hp);
		else
			phase03(hp);
		return;
	}
	di();
	hp->h_flags |= HDSEEK;
	hdselect(hdunit, hp->h_head);
	if (hp->h_trackdelta < 0) {
		hdstep(hp->h_head, 1, -hp->h_trackdelta);
		hp->h_cyl -= -hp->h_trackdelta;
	} else {
		hdstep(hp->h_head, 0, hp->h_trackdelta);
		hp->h_cyl += hp->h_trackdelta;
	}
	hdseekintr = phase02;
	hdseekarg = hp;
	ei();
}

/*
 * The seek-done continuation: the arm has settled on the cylinder, so
 * the request can be read or written.
 */
static void
phase02(hp)
	struct hd *hp;
{
	di();
	hp->h_flags &= ~HDSEEK;
	ei();
	if (hp->h_cmd == HDCA_HOME) {
		hp->h_stat = 0;
		phase12(hp);
		return;
	}
	if (hp->h_flags & HDWRITE)
		phase08(hp);
	else
		phase03(hp);
}

/*
 * Read a sector: write the header, issue the read, and let the op-done
 * interrupt pull the sector through the data port.
 */
static void
phase03(hp)
	struct hd *hp;
{
	hdselect(hdunit, hp->h_head);
	if (hp->h_cmd != HDCA_READHDR)
		hdhdr(hp->h_head, hp->h_cyl, hp->h_sector);
	di();
	hdintr = phase04;
	hdintarg = hp;
	if (hp->h_cmd == HDCA_READHDR)
		out(HDCA_CMD, CMD_READHDR);
	else
		out(HDCA_CMD, CMD_READSEC);
	ei();
}

/*
 * The op-done continuation for a read: pull the sector into the buffer
 * and finish the request.
 */
static void
phase04(hp)
	struct hd *hp;
{
	char *p;

	if (hdproc()) {
		hp->h_stat = EIO;
		phase11(hp);
		return;
	}
	p = bhold(hp->h_cur);
	if (hp->h_cmd == HDCA_READHDR)
		hdhdrrd(p);
	else
		hdrdat(p, hp->h_cur->count);
	brel();
	hp->h_stat = 0;
	phase12(hp);
}

/*
 * Write a sector: write the header and the data, issue the write, and
 * let the op-done interrupt finish the request.
 */
static void
phase08(hp)
	struct hd *hp;
{
	char *p;

	hdselect(hdunit, hp->h_head);
	p = bhold(hp->h_cur);
	if (hp->h_cmd == HDCA_WRITEHDR)
		hdhdrwr(p);
	else {
		hdhdr(hp->h_head, hp->h_cyl, hp->h_sector);
		hdwdat(p, hp->h_cur->count);
	}
	brel();
	di();
	hdintr = phase09;
	hdintarg = hp;
	if (hp->h_cmd == HDCA_WRITEHDR)
		out(HDCA_CMD, CMD_WRITEHDR);
	else
		out(HDCA_CMD, CMD_WRITESEC);
	ei();
}

/*
 * The op-done continuation for a write: the sector is on the disk.
 */
static void
phase09(hp)
	struct hd *hp;
{
	if (hdproc()) {
		hp->h_stat = EIO;
		phase11(hp);
	} else {
		hp->h_stat = 0;
		phase12(hp);
	}
}

/*
 * The request failed: mark the buffer and finish it.
 */
static void
phase11(hp)
	struct hd *hp;
{
	hp->h_flags |= HDERROR;
	hp->h_stat = EIO;
	if (hp->h_cur && !hp->h_cmd) {
		hp->h_cur->flags |= BERROR;
		hp->h_cur->error = EIO;
	}
	phase12(hp);
}

/*
 * The request finished: iodone, then start the next queued request.
 */
static void
phase12(hp)
	struct hd *hp;
{
	if (hp->h_cmd) {
		wakeup(&hdioctl);
		hp->h_cmd = 0;
	} else
		iodone(hp->h_cur);
	hp->h_buf = 0;
	hp->h_cur = 0;
	release(hp);
}

/*
 * The interrupt handler, on line 0 (shared with the HD-DMA).  INT_A
 * (op done) resumes the armed op-done phase; INT_B (seek done) resumes
 * the armed seek phase.  A primary status of 0xff is not our card.
 */
int
hdint()
{
	UINT8 psr, ssr;

	psr = in(HDCA_PSR);
	if (psr == 0xff)
		return (0);
	if (psr & PSR_OPDONE) {
		out(HDCA_CMD, CMD_RAMDATA);	/* acknowledge op-done */
		if (hdintr) {
			(*hdintr) (hdintarg);
			hdintr = 0;
			hdintarg = 0;
		}
		return (0);
	}
	ssr = in(HDCA_SSR);
	if (ssr & SSR_SDONE) {
		(void)in(HDCA_INT);	/* acknowledge seek-done */
		if (hdseekintr) {
			(*hdseekintr) (hdseekarg);
			hdseekintr = 0;
			hdseekarg = 0;
		}
	}
	return (0);
}

/*
 * Open a drive: read the disk label for its geometry, as every other
 * disk driver does.  The drive number is the low two bits of the minor.
 */
int
hdopen(dev, mode)
	UINT dev, mode;
{
	struct hd *hp;
	UINT drive;

	drive = dev & 3;
	if (drive >= NHD) {
		u.error = ENXIO;
		return (-1);
	}
	hp = getinfo(dev);
	/*
	 * Claim the interrupt line before the first read.  Line 0 is shared
	 * with the HD-DMA, which unmasks it at its own open (inton(MWINT) in
	 * sys/mw.c), and the HDCA is the boot driver on an M10/M20/M26 - the
	 * device the kernel mounts root from, so mw may never open and nothing
	 * else would clear the mask.  The 1.4 driver did the same thing after
	 * every command; here it is once, at open, the way mw.c does it.  The
	 * label read below is the first interrupt-driven read, so the line has
	 * to be armed before dllabel() and not after.
	 */
	inton(HDCAINT);
	if (hp->h_flags & HDOPEN) {
		if (hp->h_slice != devslice(dev))
			u.error = EBUSY;
		return (0);
	}
	hp->h_slice = devslice(dev);
	hp->h_tracks = 0;
	hp->h_heads = 0;
	hp->h_sectors = 0;
	hp->h_roll = 0;
	/*
	 * The label read is block 0, and the HDCA's formatter numbers the
	 * first sector 1 (FORMATHD's skew table runs 1..32), so block 0 is
	 * sector 1, not 0.  dllabel() reads block 0 with the geometry it
	 * can see - one head of one sector - so this field is what makes
	 * that read land on sector 1.  The label's own d_firstsec then
	 * overwrites it.
	 */
	hp->h_firstsec = 1;

	switch (dllabel((dev & ~0377) | (DL_WHOLE << 5) | drive,
	    devslice(dev), (struct dlgeom *)hp)) {
	case -1:
		return (-1);
	case 0:
		if (devslice(dev) != DL_WHOLE) {
			u.error = ENXIO;
			return (-1);
		}
		hp->h_heads = 1;
		hp->h_sectors = 1;
		hp->h_spc = 1;
		hp->h_maxblk = 0;
		break;
	}
	hp->h_flags |= HDOPEN;
	return (0);
}

/*
 * Close a drive: wait for any I/O to drain, then drop the open flag.
 */
int
hdclose(dev)
	UINT dev;
{
	struct hd *hp;

	hp = getinfo(dev);
	while (hp->h_flags & HDWAIT)
		sleep(hp, PRIBIO);
	hp->h_flags &= ~HDOPEN;
	return (0);
}

/*
 * Format one track: write all of its sector headers back to back.  This
 * is the one operation that is hard real time - the headers have to land
 * inside a single revolution, so no interrupt may run and nothing else
 * may take the controller between two of them.  It is one blocking call,
 * polled to completion with interrupts off; a wedge here hangs, which is
 * what the original formatter did.
 */
static int
hdfmt(hp, c, b, drive)
	struct hd *hp;
	struct hdca_cmd *c;
	struct buf *b;
	UINT drive;
{
	int spt = c->sector;		/* the sector field carries the count */
	int head = c->head;
	int track = c->track;
	char hdr[128];
	char *p;
	int i, delta, dir, n;

	if (spt < 1 || spt > 32 || !b) {
		u.error = EINVAL;
		return (-1);
	}
	/*
	 * The data phase is the whole header table - one four byte header of
	 * head, track, sector and key per sector, in the order the caller
	 * chose.  The driver writes what it is given and computes nothing;
	 * the interleave and the key are the caller's business.
	 */
	p = bhold(b);
	for (i = 0; i < spt * 4; i++)
		hdr[i] = p[i];
	brel();

	di();
	if (hdmutex) {
		ei();
		u.error = EBUSY;
		return (-1);
	}
	hdmutex = 1;

	/*
	 * Interrupts stay off from here to the end, so the op-done
	 * acknowledgment cannot be stolen from between two headers.
	 */
	hdselect(drive, head);

	delta = track - hp->h_cyl;
	if (delta) {
		dir = (delta < 0) ? 1 : 0;
		n = (delta < 0) ? -delta : delta;
		hdstep(head, dir, n);
		hp->h_cyl = track;
		while (!(in(HDCA_PSR) & PSR_COMPLT))
			;
	}

	for (i = 0; i < spt; i++) {
		hdhdrwr(hdr + i * 4);
		out(HDCA_CMD, CMD_WRITEHDR);
		while (!(in(HDCA_PSR) & PSR_OPDONE))
			;
	}

	ei();
	hdmutex = 0;
	return (0);
}

/*
 * A command block from above (include/sys/ioctl.h).  The HDCA has no
 * command block of its own - it is port-mapped - so the block is the
 * driver's invention (include/sys/hdca.h): an opcode and the head, track
 * and sector to seek to, with the data phase, when the op has one, in
 * the buffer bioctl() staged it in.  The command runs through the same
 * interrupt-driven phases a read or a write does - the ioctl sleeps and
 * the op-done interrupt wakes it through phase12() - and the completion
 * status comes back in the byte after the block, cmd[HDLEN].
 */
int
hdcioctl(dev, cmd, r, b)
	UINT dev, cmd;
	struct cdb *r;
	struct buf *b;
{
	struct hdca_cmd *c;
	struct hd *hp;
	UINT drive;

	if (cmd != CDBCMD) {
		u.error = ENXIO;
		return (-1);
	}
	c = (struct hdca_cmd *) r->cmd;
	if (r->len != HDLEN || c->op < HDCA_HOME || c->op > HDCA_FORMAT) {
		u.error = EINVAL;
		return (-1);
	}
	drive = dev & 3;
	hp = &hdinfo[drive];
	if (!(hp->h_flags & HDOPEN)) {
		u.error = ENXIO;
		return (-1);
	}

	/*
	 * A format is the whole track in one blocking, polled call; it does
	 * not go through the interrupt-driven phases the other ops sleep on.
	 */
	if (c->op == HDCA_FORMAT)
		return (hdfmt(hp, c, b, drive));

	/*
	 * The controller serves one request at a time and the ioctl is a
	 * request: take the mutex the way exclude() would, but fail rather
	 * than queue, the way the HD-DMA's ioctl does (sys/mw.c).
	 */
	di();
	if (hdmutex) {
		ei();
		u.error = EBUSY;
		return (-1);
	}
	hdmutex = 1;
	ei();

	hp->h_buf = b;
	hp->h_cur = b;
	hp->h_head = c->head;
	hp->h_sector = c->sector;
	hp->h_trackdelta = c->track - hp->h_cyl;
	hp->h_cmd = c->op;
	if (c->op == HDCA_WRITEHDR || c->op == HDCA_WRITESEC)
		hp->h_flags |= HDWRITE;
	else
		hp->h_flags &= ~HDWRITE;

	phase00(hp);

	while (hp->h_cmd)
		sleep(&hdioctl, PRIBIO);

	r->cmd[HDLEN] = hp->h_stat;
	return (0);
}

struct biovec hdbvec = { &hdopen, &hdclose, &hdstrat, &hdcioctl };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
