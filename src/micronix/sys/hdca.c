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

extern int in(), out(), timeout();

struct hd hdinfo[NHD];

static int hdmutex;		/* the controller, 0 = free */
static int (*hdintr) ();	/* the phase the op-done interrupt resumes */
static struct hd *hdintarg;	/* its argument */
static int (*hdseekintr) ();	/* the phase the seek-done interrupt resumes */
static struct hd *hdseekarg;	/* its argument */
static UINT8 hdunit;		/* the drive the FUNC register selects */

static void phase00(), phase01(), phase02(), phase03(), phase04(),
    phase06(), phase08(), phase09(), phase11(), phase12(), hdtimeout();
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
	out(HDCA_FUNC, (drive & FUNC_DRIVE) |
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

	fn = (hdunit & FUNC_DRIVE) | (((head ^ 0x0f) << 4) & FUNC_HEAD);
	if (dir)
		fn |= FUNC_DIR;
	for (i = 0; i < n; i++) {
		out(HDCA_FUNC, fn | FUNC_STEP);
		out(HDCA_FUNC, fn);
		out(HDCA_FUNC, fn | FUNC_STEP);
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
	timeout(hdtimeout, hp, HDTIMEOUT);
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
	if (hp->h_flags & HDWRITE)
		phase08(hp);
	else
		phase03(hp);
}

/*
 * The seek deadline.  A seek that finished in time cleared HDSEEK, so a
 * late deadline tick sees the flag off and does nothing - it is shot
 * down.  A seek whose done-interrupt never came still has HDSEEK set,
 * and that is a wedged drive: the request fails with EIO.
 */
static void
hdtimeout(hp)
	struct hd *hp;
{
	di();
	if (!(hp->h_flags & HDSEEK)) {
		ei();
		return;
	}
	hp->h_flags &= ~HDSEEK;
	hdseekintr = 0;
	hdseekarg = 0;
	ei();
	phase11(hp);
}

/*
 * Read a sector: write the header, issue the read, and let the op-done
 * interrupt pull the sector through the data port.
 */
static void
phase03(hp)
	struct hd *hp;
{
	hdhdr(hp->h_head, hp->h_cyl, hp->h_sector);
	di();
	hdintr = phase04;
	hdintarg = hp;
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
		phase11(hp);
		return;
	}
	p = bhold(hp->h_cur);
	hdrdat(p, hp->h_cur->count);
	brel();
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

	hdhdr(hp->h_head, hp->h_cyl, hp->h_sector);
	p = bhold(hp->h_cur);
	hdwdat(p, hp->h_cur->count);
	brel();
	di();
	hdintr = phase09;
	hdintarg = hp;
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
	if (hdproc())
		phase11(hp);
	else
		phase12(hp);
}

/*
 * The request failed: mark the buffer and finish it.
 */
static void
phase11(hp)
	struct hd *hp;
{
	hp->h_flags |= HDERROR;
	hp->h_cur->flags |= BERROR;
	hp->h_cur->error = EIO;
	phase12(hp);
}

/*
 * The request finished: iodone, then start the next queued request.
 */
static void
phase12(hp)
	struct hd *hp;
{
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
 * No ioctl: the controller has nothing to be told and nothing to ask.
 */
int
hdcioctl(dev, cmd, r, b)
	UINT dev, cmd;
	struct cdb *r;
	struct buf *b;
{
	u.error = ENXIO;
	return (-1);
}

struct biovec hdbvec = { &hdopen, &hdclose, &hdstrat, &hdcioctl };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
