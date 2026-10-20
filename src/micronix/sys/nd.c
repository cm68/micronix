/*
 * The network disk driver.  sys/nd.c
 *
 * A block device whose strat() proxies block I/O to a network card the
 * way sys/hdca.c proxies it to the HDCA: arm a continuation, issue the
 * command, return; the op-done interrupt resumes the armed phase and the
 * 512 bytes move through the data port.  What a disk driver does that is
 * not here - seek, geometry, headers - is because a disk has moving parts
 * and a label; a network block device has neither, only a flat block
 * number and a card that fetches or stores it.
 *
 * The card serves one request at a time (hwsim/d1/nic.c), so one request
 * runs and the rest queue on the single drive, exactly as the HDCA's
 * requests queue on their drives.
 */

#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/nd.h>
#include <errno.h>

extern int in(), out(), inton();

struct nd ndinfo[NND];

static int ndmutex;		/* the card, 0 = free */
static int (*ndintr) ();	/* the phase the op-done interrupt resumes */
static struct nd *ndintarg;
static UINT8 nddead;		/* seconds the request has been in flight */

static void phase00(), phase04(), phase09(), phase11(), phase12(), release();

/*
 * One byte in and out of the card's data area at the current pointer.
 */
static UINT8
ndin()
{
	return (in(ND_DATA));
}

static void
ndout(b)
	UINT8 b;
{
	out(ND_DATA, b);
}

/*
 * Point the card at the block number and write it - four bytes, most
 * significant first, the way the HDCA's header is four bytes.
 */
static void
ndblk(blk)
	UINT blk;
{
	out(ND_CMD, ND_BLOCK);
	ndout((blk >> 24) & 0xff);
	ndout((blk >> 16) & 0xff);
	ndout((blk >> 8) & 0xff);
	ndout(blk & 0xff);
}

/*
 * Move a block between the card and the buffer.  bhold() has mapped the
 * buffer's segment and handed back its data address.
 */
static void
ndrdat(p)
	register char *p;
{
	register UINT n;

	out(ND_CMD, ND_RAMDATA);
	n = 512;
	while (n--)
		*p++ = ndin();
}

static void
ndwdat(p)
	register char *p;
{
	register UINT n;

	out(ND_CMD, ND_RAMDATA);
	n = 512;
	while (n--)
		ndout(*p++);
}

/*
 * Drive arbitration, the HDCA's exclude()/release() without the drive
 * table: the card serves one request at a time, and a request that finds
 * it busy is queued on the single drive.
 */
static int
exclude(np)
	struct nd *np;
{
	if (!ndmutex) {
		ndmutex = 1;
		return (0);
	}
	np->n_flags |= NDWAIT;
	np->n_next = np->n_buf;
	return (-1);
}

static void
release(np)
	struct nd *np;
{
	if (np->n_next) {
		np->n_buf = np->n_next;
		np->n_next = 0;
		np->n_flags &= ~NDWAIT;
		phase00(np);
		return;
	}
	ndmutex = 0;
}

/*
 * The strategy entry: a block I/O from the buffer cache.
 */
int
ndstrat(b)
	struct buf *b;
{
	struct nd *np;

	np = &ndinfo[0];		/* one drive */
	np->n_buf = b;
	np->n_cur = b;
	if (exclude(np) < 0)
		return (0);
	phase00(np);
	return (0);
}

/*
 * Begin a request: write the block number, issue the read or write, and
 * arm the op-done phase.
 */
static void
phase00(np)
	struct nd *np;
{
	struct buf *b = np->n_cur;
	char *p;

	np->n_flags &= ~NDERROR;
	nddead = 0;
	ndblk(b->blk);
	di();
	if (b->flags & BREAD) {
		out(ND_CMD, ND_READ);
		ndintr = phase04;
	} else {
		p = bhold(b);
		ndwdat(p);
		brel();
		out(ND_CMD, ND_WRITE);
		ndintr = phase09;
	}
	ndintarg = np;
	ei();
}

/*
 * The op-done continuation for a read: pull the block into the buffer
 * and finish the request.
 */
static void
phase04(np)
	struct nd *np;
{
	char *p;

	if (in(ND_STATUS) & ND_ERROR) {
		phase11(np, EIO);
		return;
	}
	p = bhold(np->n_cur);
	ndrdat(p);
	brel();
	np->n_stat = 0;
	phase12(np);
}

/*
 * The op-done continuation for a write: the block is stored.
 */
static void
phase09(np)
	struct nd *np;
{
	if (in(ND_STATUS) & ND_ERROR)
		phase11(np, EIO);
	else {
		np->n_stat = 0;
		phase12(np);
	}
}

/*
 * The request failed: mark the buffer and finish it.
 */
static void
phase11(np, err)
	struct nd *np;
	int err;
{
	np->n_flags |= NDERROR;
	np->n_stat = err;
	if (np->n_cur && !np->n_cmd) {
		np->n_cur->flags |= BERROR;
		np->n_cur->error = err;
	}
	phase12(np);
}

/*
 * The request finished: iodone, then start the next queued request.
 */
static void
phase12(np)
	struct nd *np;
{
	if (np->n_cmd)
		np->n_cmd = 0;
	else
		iodone(np->n_cur);
	np->n_buf = 0;
	np->n_cur = 0;
	release(np);
}

/*
 * The watchdog.  A request whose op-done was never processed - because
 * the line is masked, or because the card has stopped answering - sits
 * here past NDDEADLINE seconds and is failed with ENXIO, the only way it
 * ever ends.  The probe reads a drive's label before the drive is opened,
 * so this is what turns that silence into a skip.
 */
int
ndtick()
{
	struct nd *np;

	if (!ndmutex)
		return (0);
	if (++nddead <= NDDEADLINE)
		return (0);
	di();
	ndintr = 0;
	ndintarg = 0;
	ei();
	np = &ndinfo[0];
	if (np->n_cur)
		phase11(np, ENXIO);
	return (0);
}

/*
 * The interrupt handler, on the DISK3 spare line.  Op-done resumes the
 * armed phase.
 */
int
ndint()
{
	if (!(in(ND_STATUS) & ND_OPDONE))
		return (0);
	if (ndintr) {
		(*ndintr) (ndintarg);
		ndintr = 0;
		ndintarg = 0;
	}
	return (0);
}

/*
 * Open the drive.  There is no label to read and no geometry to learn.
 */
int
ndopen(dev, mode)
	UINT dev, mode;
{
	return (0);
}

/*
 * Close the drive: wait for any I/O to drain.
 */
int
ndclose(dev)
	UINT dev;
{
	struct nd *np;

	np = &ndinfo[0];
	while (np->n_flags & NDWAIT)
		sleep(np, PRIBIO);
	return (0);
}

/*
 * The ND has no command block: it is a flat block device, and every
 * control operation a disk needs - format, seek, geometry - is
 * meaningless here.
 */
int
ndcioctl(dev, cmd, r, b)
	UINT dev, cmd;
	struct cdb *r;
	struct buf *b;
{
	u.error = ENOTTY;
	return (-1);
}

struct biovec ndbvec = { &ndopen, &ndclose, &ndstrat, &ndcioctl };

/*
 * The probe: make the card raise its interrupt so the attach loop
 * (sys/probe.c) can discover which line it is on.  STUB - not written;
 * it must issue a command that completes and raises the card's done bit.
 */
ndprobe()
{
    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
