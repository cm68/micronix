/*
 * Disk Jockey DMA driver
 *
 * sys/dj.c
 *
 * The driver is two things and nothing else: a 512-byte transfer engine,
 * and the door a caller pushes a controller program through (djioctl).
 * A floppy's geometry comes from its own disk label, read with dllabel()
 * the way mw and ide read a hard disk's; the one property the label does
 * not carry - whether the medium numbers its sectors from one or zero -
 * is read from the drive itself at open, as is write protect.  Everything
 * that used to be here to sniff the format of a foreign diskette - the
 * specs[] table, the delay[] table, getstat's IBM and Morrow probing - is
 * gone: that is djformat's and far's business, and they reach the board
 * through the same command block the driver does.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <errno.h>
#include <sys/dj.h>
#include <sys/ioctl.h>
#include <sys/ovl.h>
#include <sys/dlabel.h>
#include <sys/pic.h>

/*
 * The static routines below, declared before anything calls them.
 */
static int sio(), djwait(), djstart(), getstat();
static int memio(), busready(), busplease(), busthanks();

/*
 * From the kernel rather than from a header: the staging page's
 * allocator, the raw user-memory copies, the raw window mapping, and the
 * disk label decoder.
 */
extern int segalloc();
extern UINT8 map0[], bufseg;
extern int copyin(), copyout();
extern bwin();
extern int dllabel();
extern char *swin();
extern void srel();

/*
 * This driver's initialization is not in this object: it is init-only
 * code folded into the reclaimed region beside cus() (sys/djinit.c).  The
 * driver's header (sys/djhdr.c) is what names it, so this file does not
 * have to.
 */

static unsigned char *haltstat = 0, djtimer = 0;

unsigned char djcomm[26] = { 0 };

/*
 * The staging page.
 *
 * The DMA controller has no scatter/gather: it is handed one address and
 * moves a whole transfer to or from it, so anything bigger than a 512
 * byte block must be contiguous in physical memory.  The buffer cache's
 * blocks are not guaranteed to be; this page, a single segalloc() 4K
 * segment, always is.  It is the data phase of the ioctl door - a
 * formatter writing a track, far reading a raw CP/M sector - and of
 * getstat's STATUS read.  The 512 byte transfer path does not use it: a
 * block is one sector and the DMA goes straight to the buffer.
 *
 * It is taken on the first open and never given back: one page of the
 * 256, against a driver that can be reopened at any moment.
 */
static UINT8 kseg = 0;          /* 0 = not taken; segalloc() starts at 16 */

static UINT8
kstg()
{
	if (!kseg)
		kseg = (UINT8) segalloc();
	return (kseg);
}

/*
 * Per-drive state.  The geometry is the label's own, decoded into a
 * struct dlgeom (sys/dlabel.h), which dllabel() fills directly.
 */
struct dm
{
	struct dlgeom g;            /* the label's geometry and timing, decoded */
	UINT8 flags;                /* ISOPEN | F_WP */
	UINT8 k;                    /* 1K sectors: a 512 block is half a sector */
	UINT8 fm;                   /* eight inch drive: block 0 is fm */
};

static struct dm dm[NDRIVES] = { 0 };

static struct buf *curbuf = 0,  /* buffer currently under scrutiny */
	*qhead = 0,                 /* New requests added head. */
	*qtail = 0;                 /* Next request found here. */

/*
 * The step and settle delays, written to the controller's per-drive table
 * at open.  One fixed pair for the one drive this machine has - the 5.25
 * inch default, 5 ms step and 15 ms settle - in place of the per-drive-
 * model delay[] table that went away with the format detection.
 */
#define DSTEP   170             /* 5 ms, in the board's delay ticks */
#define DSETTLE 511             /* 15 ms */

djopen(dev, mode)
	register dev, mode;
{
	struct dm *d;
	struct dlgeom *g;
	struct status *s;
	unsigned drive, sl;

	/*
	 * The staging page, before anything can want it.  This is the one
	 * place it is taken: djopen() is process context, so segalloc()'s
	 * di()/ei() is safe here and not in the handler - see kstg().
	 */
	kstg();

	drive = dev & 7;
	d = &dm[drive];

	if (d->flags & ISOPEN)      /* already open */
		return 0;

	/*
	 * Read the drive's status first - it sets the step and settle delays
	 * and brings the head up - so the label read that follows has a
	 * settled drive under it.  Write protect comes from here too.
	 */
	if (getstat(dev) == 0) {
		u.error = EIO;
		return;
	}

	s = (struct status *) djcomm;

	/*
	 * The sector length is the medium's, and it is the one thing the
	 * label does not carry: a 1K sector means a 512 byte block is half
	 * a sector, read and written as a read-modify-write.  It has to be
	 * known before dllabel(), whose read of block 0 is itself a block
	 * transfer through this same path.
	 */
	d->k = (s->slength == S1024);

	/*
	 * An eight inch drive's cylinder 0 head 0 is single density - 26
	 * sectors of 128 bytes - and block 0, where the label is, is the
	 * first four of them.  A 512 byte SREAD cannot fetch that block:
	 * one sector read returns one sector, 128 bytes of it.  The drive's
	 * characteristics byte is how the board says which it is - FIVE set
	 * for the 5 1/4 inch port - so a clear one is an eight inch drive
	 * and its block 0 is four reads.  See fmio() and djint().
	 */
	d->fm = (s->dchar & FIVE) == 0;

	if (s->dstat & S_WP) {      /* write protect */
		d->flags |= F_WP;
		if (mode != READ) {
			u.error = EROFS;
			return;
		}
	} else
		d->flags &= ~F_WP;

	/*
	 * The geometry, from the disk's own label.  What is left in dm[]
	 * from a previous open is not knowledge: the disk may have been
	 * changed, and its label went with it.
	 *
	 * The first sector number is the one thing that has to be known
	 * before the label can be read: the label itself lives in a sector
	 * of the boot track, and whether that track numbers from zero or
	 * one is a property of the medium, which the drive's characteristics
	 * byte (dchar, HARD) reports and the label merely records for the
	 * loader, which has no drive to ask.  Set it here so dllabel()'s own
	 * read of block 0 lands on the sector the label is actually in.
	 */
	g = &d->g;
	g->tracks = 0;
	g->heads = 0;
	g->sectors = 0;
	g->roll = 0;
	g->firstsec = (s->dchar & HARD) ? 0 : 1;
	sl = devslice(dev);
	if (dllabel((dev & ~0377) | (DL_WHOLE << 5) | drive, sl, g, 0, 0) < 0)
		return;

	d->flags |= ISOPEN;
}

/*
 * Close the drive, disallowing further access.
 */

djclose(dev)
	register dev;
{
	dm[dev & 7].flags = 0;
}

/*
 * link the buffer into the queue
 */

djstrat(b)
	register struct buf *b;
{
	b->forw = 0;

	di();

	if (qtail) {
		qhead->forw = b;
		qhead = b;
	} else {                    /* No request yet. */
		qhead = qtail = b;
	}

	djint();

	ei();
}

/*
 * Lay out the command for one 512 byte sector and hand it to the
 * controller.  sec is the absolute sector number; the geometry that turns
 * it into a track, a head and a sector within the track comes from the
 * label (struct dlgeom above), not a table.
 */
static
sio(d, addr, xaddr, op, sec, dev)
	register unsigned sec;
	register struct dm *d;
{
	static struct dlgeom *g;
	static unsigned track, within;
	static UINT32 phys;

	g = &d->g;

	/*
	 * The board wants a flat 24-bit physical address, and the caller has
	 * the buffer as a window address and a 4K segment - the kernel's own
	 * view of it, the same pair disk1 and uio use.  Unfold it: the
	 * segment's twelve bits start where the window address stops.
	 */
	phys = ((UINT32) xaddr << 12) | ((unsigned) addr & 0xfff);

	D[0] = SETDMA;              /* set DMA command */
	D[1] = phys;                /* D[1], D[2], D[3]: */
	D[2] = phys >> 8;           /* low, mid and the */
	D[3] = phys >> 16;          /* extended page */

	D[4] = op;                  /* I/O command */

	/*
	 * The track is the cylinder, rolled and sliced the way dllabel taught
	 * it: the block's cylinder, rotated by the slice's own roll within
	 * the slice, then shifted by the slice's offset.
	 */
	track = sec / g->spc + g->roll;
	if (track >= g->len)
		track -= g->len;
	track += g->cylstart;
	D[5] = track;

	/*
	 * The sector within the track, the head folded into its high bit
	 * (VERSO) because the board has no separate head field, and the
	 * medium's first-sector number.
	 */
	within = sec % g->spc;
	D[6] = (within % g->sectors) + g->firstsec;
	if (d->k)
		D[6] = ((within % g->sectors) >> 1) + g->firstsec;
	if (g->heads > 1 && within >= g->sectors)
		D[6] |= VERSO;

	D[7] = dev & 7;             /* drive */

	D[8] = NOSTAT;              /* io status will be returned here */

	D[9] = SETINT;
	D[10] = NOSTAT;

	D[11] = HALT;

	haltstat = &D[12];

	/* a command is 13 bytes long */
}

/*
 * One of the four 128 byte sectors of an eight inch drive's block 0.
 *
 * The block is on the single density track at cylinder 0 head 0, and the
 * geometry cannot map it: dllabel()'s read happens before any geometry
 * exists, and even after the label is read the block is not a 512 byte
 * sector that a block number divides into.  So n is the sector within
 * that track - 0 through 3, the medium's first sector plus n - and the
 * DMA address is the block's buffer plus n*128.  The caller starts the
 * four in turn; djint() runs the next when one completes.
 */
static
fmio(d, addr, xaddr, op, n, dev)
	register struct dm *d;
	unsigned addr, xaddr;
	unsigned op, n, dev;
{
	static UINT32 phys;

	phys = ((UINT32) xaddr << 12) | (addr & 0xfff);

	D[0] = SETDMA;
	D[1] = phys;
	D[2] = phys >> 8;
	D[3] = phys >> 16;

	D[4] = op;
	D[5] = 0;                   /* cylinder 0 */
	D[6] = d->g.firstsec + n;   /* sectors 1 through 4 */

	D[7] = dev & 7;
	D[8] = NOSTAT;

	D[9] = SETINT;
	D[10] = NOSTAT;

	D[11] = HALT;

	haltstat = &D[12];
}

/*
 * wait a little while to see if the given location changes
 */

static
djwait(a)
	char *a;
{
	unsigned int i;

	for (i = TIME; i && *a == NOSTAT; i--);
}

static unsigned char inservice = 0;
static unsigned char fmsec = 0; /* which fm sector of block 0 is in flight */

djint()
{
	static unsigned char op, kw;
	static unsigned addr, xaddr, sec, koff, dev;
	static struct dm *d;
	static struct dlgeom *g;

	pr("");                     /* timing delay kludge */

	di();

	if (inservice               /* already working on it */
		|| (curbuf && INTSTAT == NOSTAT)) {     /* not our int. */
		ei();
		return;
	}

	if (!curbuf && !qtail) {    /* nothing in flight */
		/*
		 * The attach probe's tickle is the one interrupt with no
		 * request behind it: the card raised its line to say where
		 * it is wired (djprobe), and the line stays up until the
		 * board is acknowledged.  Dismiss it, or the next
		 * completion's rising edge never comes.
		 */
		if (haltstat)
			djack();
		ei();
		return;
	}

	if (busget(&djint) == 0) {  /* can't get bus yet. See mw.c */
		ei();
		return;
	}

	inservice = 1;
	ei();

	djtimer = 0;                /* reset the deadman switch */

	/*
	 * first, check up on the current operation
	 */

	if (curbuf) {               /* something happening */
		if (IOSTAT != OKSTAT) { /* failed */
		  fail:
			curbuf->error = EIO;
		  berr:
			curbuf->flags |= BERROR;

		  finished:
			iodone(curbuf);
			curbuf = 0;

			/*
			 * acknowledge the interrupt: push the board past the
			 * interrupt command, then drop the bus.
			 */
			djack();

			if (!qtail)
				goto request;

			if (busgive(&djint) || !busget(&djint)) {
				inservice = 0;
				ei();
				return;
			}

			goto request;       /* set up next req. */
		}

		if (d->fm && sec == 0) {
			/*
			 * Block 0 of an eight inch drive: four 128 byte
			 * sectors, one command each.  The next starts here
			 * when this one completes, so the last falls through
			 * to finished with the whole block in the buffer.
			 */
			if (fmsec < 3) {
				fmsec++;
				djack();        /* acknowledge this sector */
				fmio(d, curbuf->data + fmsec * 128, curbuf->xmem,
				    op, fmsec, dev);
				di();
				djstart();
				inservice = 0;
				ei();
				return;
			}
			goto finished;
		}

		if (d->k && op == SWRITE) {
			/*
			 * A 1K sector write is a read-modify-write: the read
			 * above brought the whole sector in, so fold this block
			 * into its half and write the sector back.
			 */
			if (kw)             /* the write-back just finished */
				goto finished;
			/*
			 * bhold() returns the block's window address and reads
			 * curbuf->xmem, both of which have to be taken now: swin()
			 * below remaps the page the buffer headers live in, so a
			 * curbuf read after it is the sector just read, not the
			 * header.  The address is what the copy needs; the segment
			 * was written to BUFSEG by bhold itself.
			 */
			addr = bhold(curbuf);
			swin(kseg);
			copy(addr, (char *) SCRWIN + koff, BUFSIZE);
			srel();
			brel();
			kw = 1;
			djack();            /* acknowledge the read */
			sio(d, 0, kseg, SWRITE, sec, dev);
			di();
			djstart();
			inservice = 0;
			ei();
			return;
		}

		if (d->k && op == SREAD) {
			/*
			 * A 1K sector read: copy the block's half out of the
			 * sector just read.  As in the write above, the block's
			 * address comes off the header before swin() remaps the
			 * headers' page.
			 */
			addr = bhold(curbuf);
			swin(kseg);
			copy((char *) SCRWIN + koff, addr, BUFSIZE);
			srel();
			brel();
		}

		/*
		 * the operation worked; a 512 byte transfer is one sector and
		 * is already where it belongs.
		 */
		goto finished;
	}

	/*
	 * nothing happening, pluck a buffer
	 */

  request:

	di();

	if (!qtail) {               /* all done, no more requests */
		inservice = 0;
		ei();
		busgive(0);
		return;
	}

	curbuf = qtail;
	qtail = qtail->forw;

	ei();

	dev = curbuf->dev;

	if (dev & GETSTAT) {        /* stat req. */
		if (getstat(dev)) {
			copy(djcomm, bhold(curbuf), sizeof(struct status));
			brel();
			goto finished;
		} else {
			goto fail;
		}
	}

	d = &dm[dev & 7];

	/*
	 * A read may reach an unopened drive: dllabel() reads block 0 of the
	 * drive's 'c' while djopen is still deciding, and that block has to
	 * be readable before the open exists to allow it.  The bound below is
	 * the real gate - an unopened drive has maxblk zero, so only block 0
	 * (the label) is reachable, which is the same view mw.c takes.
	 */
	g = &d->g;

	if (curbuf->blk > g->maxblk) {      /* off the end of the slice */
		curbuf->error = ENXIO;
		goto berr;
	}

	op = (curbuf->flags & BREAD) ? SREAD : SWRITE;

	if (op == SWRITE && (d->flags & F_WP))
		goto fail;

	sec = curbuf->blk;

	if (d->k) {
		/*
		 * One block is half of a 1K sector.  Both directions read the
		 * whole sector into the staging page first: a read copies the
		 * half out, a write folds the block in and writes it back.
		 */
		koff = (sec & 1) * BUFSIZE;
		kw = 0;
		sio(d, 0, kseg, SREAD, sec, dev);
	} else {
		/*
		 * One block is one 512 byte sector, so the sector number is
		 * the block number.  The DMA goes straight to the buffer.
		 *
		 * Block 0 of an eight inch drive is the exception: it is four
		 * 128 byte sectors on the single density track.  fmio() reads
		 * the first and djint() the rest.
		 */
		addr = curbuf->data;
		xaddr = curbuf->xmem;
		if (d->fm && sec == 0) {
			fmsec = 0;
			fmio(d, addr, xaddr, op, 0, dev);
		} else
			sio(d, addr, xaddr, op, sec, dev);
	}

	di();
	djstart();
	inservice = 0;
	ei();
}

static
djstart()
{
	di();

	if (haltstat)
		*haltstat = NOSTAT;

	out(START, 0);
	ei();
}

/*
 * Read the drive's status and set its step and settle delays.  This is
 * the whole of getstat now: the format sniffing that used to follow -
 * the config byte, the IBM FAT, the Morrow checksum - is gone.  What is
 * wanted is the characteristics byte (hard or soft sectored, for ORG1)
 * and the write protect bit, which the status carries.
 */
static
getstat(a)
	register a;
{
	static unsigned astep, asettle;
	static int step = DSTEP, settle = DSETTLE;

	a &= 7;                     /* the drive select, not the full dev */

	/*
	 * One fixed step and settle pair, written to the controller's
	 * per-drive table.  The drive is the machine's own and its timing
	 * does not vary with the diskette.
	 */
	astep = TABADDR + (a << 4) + STEP;
	asettle = TABADDR + (a << 4) + SETTLE;
	memio(WRITE, &step, astep, sizeof step);
	memio(WRITE, &settle, asettle, sizeof settle);

	/*
	 * The status, read the way it always was: a SETDMA to the staging
	 * page, a seek read to cylinder 3 to bring the drive up, then the
	 * STATUS command, whose answer lands in djcomm and is read back as a
	 * struct status.
	 */
	D[0] = SETDMA;
	D[1] = ((UINT32) kseg << 12) & 0xff;    /* the page as a flat */
	D[2] = ((UINT32) kseg << 12) >> 8;      /* address: its low twelve */
	D[3] = kseg >> 4;                       /* bits are zero already */

	D[4] = SREAD;               /* move to cyl 3 */
	D[5] = 3;
	D[6] = 1;
	D[7] = a;
	D[8] = NOSTAT;

	D[9] = STATUS;              /* get stat */
	D[10] = a;

	D[14] = NOSTAT;

	D[15] = HALT;
	D[16] = NOSTAT;

	djstart();
	ei();
	djwait(&D[16]);
	di();

	if (D[14] != OKSTAT)
		return 0;

	return 1;
}

/*
 * The controller's own memory, written for the step and settle setup.
 * getstat() is the only caller now; a formatter's reach into controller
 * memory goes through the command block like everything else (djioctl).
 */
static
memio(flag, memaddr, djaddr, count)
{
	D[0] = (flag == READ) ? MEMREAD : MEMWRITE;

	/*
	 * The kernel's own memory is below 64K and identity mapped, so its
	 * address is already flat and the extended page is zero - which is
	 * what KERNEL names.
	 */
	*(char **) (D + 1) = memaddr;

	D[3] = KERNEL;

	*(int *) (D + 4) = count;

	*(char **) (D + 6) = djaddr;

	D[8] = HALT;
	D[9] = NOSTAT;

	djstart();
	djwait(&D[9]);
}

/*
 * tell the PIC we're done with the current interrupt
 */

djack()
{
	if (!haltstat)
		return;

	djstart();                  /* get past the int. command */
	djwait(haltstat);
	haltstat = 0;
}

/*
 * The deadman switch, run on the resident tick (sys/ovl.c).  A command
 * list is outstanding while haltstat is set, and the board is supposed to
 * raise an interrupt for it; djint clears djtimer every time it runs.
 * DJTHRESHHOLD periods with a list outstanding and no interrupt serviced
 * is a controller that has stopped talking, and the operation in flight
 * is failed rather than waited on.
 */
djgoose()
{
	if (haltstat && ++djtimer >= DJTHRESHHOLD) {
		IOSTAT = BADSTAT;
		INTSTAT = OKSTAT;
		djint();
	}
}

static char djticker = 0;

djtick()
{
	if (++djticker < DJTICKS)
		return (0);
	djticker = 0;
	return (djgoose());
}

/*
 * A command block from above (include/sys/ioctl.h).
 *
 * The block is a program written in the controller's own instructions
 * (sys/dj.h), and the driver runs it as it stands: the bytes go to
 * djcomm, the address of the data phase is planted where the caller said
 * it goes, and the controller is started.  Not one opcode is looked at.
 * That is the whole of the contract, and it is what lets a formatter -
 * djformat, far, and eventually dar - use the same door a sector read
 * does.
 *
 * The address is the caller's one hole: it knows its buffer as a virtual
 * address and the board wants a physical one, so req.hole says where in
 * the program the three bytes go and the driver writes them (three, not
 * two, because the board's address is 24 bits wide).
 *
 * The data phase has two shapes.  A block's worth arrives in b, the
 * pooled buffer bioctl staged it in, and the board is pointed at the
 * window it is mapped into; bhold() returns the address with the mapping
 * up, so the window is given back here.  More than a block arrives in
 * nothing and is staged in the page getstat and the big transfers share.
 */
int
djioctl(dev, cmd, r, b)
	UINT dev, cmd;
	register struct cdb *r;
	struct buf *b;
{
	static char *p;             /* the staging, as the kernel reaches it */
	static UINT32 phys;         /* ... and as the board is told it */
	static UINT8 save;
	static int i, len, count, hole, flags, st;

	/*
	 * The block's own words come off the caller's structure once; this
	 * is a page with every byte counted.
	 */
	len = r->len;
	count = r->count;
	hole = r->hole;
	flags = r->flags;

	if (len < 2 || len > sizeof djcomm || count < 0 || count > CDBDATA
		|| (hole >= 0 && hole + 3 > len)) {
		u.error = EINVAL;
		return (-1);
	}

	if (count > 512) {
		kstg();
		save = bufseg;
		bwin(kseg);
		p = (char *) BUFWIN;
		phys = ((UINT32) kseg << 12) | ((unsigned) p & 0xfff);

		if ((flags & CDB_IN) == 0)
			copyin(r->buf, p, count);

	} else if (count) {
		p = bhold(b);
		phys = ((UINT32) b->xmem << 12) | ((unsigned) p & 0xfff);

	} else {
		p = 0;
		phys = 0;
	}

	for (i = 0; i < len; i++)
		D[i] = r->cmd[i];

	/*
	 * The three bytes are the address the way the controller's SETDMA
	 * takes it: one flat 24-bit address, low byte first, whose third byte
	 * is the extended page (the DJ-DMA manual's term for A16-A23), which
	 * is what SETCHANNEL in djinit.c lays down too.  The caller's hold on
	 * the buffer is a window address and a 4K segment, so the segment has
	 * to be folded into the address here rather than written beside it.
	 */
	if (hole >= 0) {
		D[hole] = phys;
		D[hole + 1] = phys >> 8;
		D[hole + 2] = phys >> 16;
	}

	busplease();

	haltstat = &D[len - 1];
	djstart();
	djwait(haltstat);

	busthanks();

	st = *haltstat;

	for (i = 0; i < len; i++)
		r->cmd[i] = D[i];

	if (count > 512) {
		if (st != NOSTAT && (flags & CDB_IN))
			copyout(p, r->buf, count);

		bwin(save);

	} else if (count) {
		brel();
	}

	if (st == NOSTAT) {         /* the controller never answered */
		u.error = EIO;
		return (-1);
	}
	return (0);
}

static
busready()
{
	wakeup(busready);
}

/*
 * get the bus - in line
 */

static
busplease()
{
	di();

	if (!busget(busready))
		sleep(busready, DJPRIORITY);
	else
		ei();
}

static
busthanks()
{
	busgive(0);
}

/*
 * The entries the driver's header hands the kernel (sys/djhdr.c), which
 * is where the header is and why it is not here: a module's first bytes
 * are the header's, and an object's data is placed in the order the
 * objects are named, so the header is alone in the object named first.
 * There is no char vector: the floppy is a block device and nothing else.
 */
struct biovec djbvec = { &djopen, &djclose, &djstrat, &djioctl };

/*
 * The probe: make the card raise its interrupt so the attach loop
 * (sys/probe_init.c) can discover which line it is on.
 *
 * The board raises its interrupt when the program it is running asks for
 * one; SETINT is that request, and the board answers it when it has
 * finished the program - on its own processor, milliseconds later.  So
 * the tickle is a two command program, the SETINT and the HALT that the
 * acknowledge steps over to, and the wait below is for the line to come
 * up where the attach loop will look for it (pic_irr).
 *
 * The line is left up on purpose.  The attach loop eois the PIC and
 * unmasks the line, and the interrupt that follows is this driver's own
 * handler with nothing to do; djint() dismisses the card there, because
 * the START pulse is what the board wants before it will raise another
 * interrupt.  Clearing it here would take the request bit with it: the
 * 8259s are level triggered, and a line taken low clears its pending
 * bit before the attach loop can read it.
 *
 * A machine with no card never answers, and this returns after the
 * bound: the attach loop finds no line and leaves it unwired.  The
 * tickle is the presence test.
 */
#define PROBEWAIT 10000

djprobe()
{
	register int n;
	int before;

	D[0] = SETINT;
	D[1] = NOSTAT;
	D[2] = HALT;
	D[3] = NOSTAT;

	haltstat = &D[3];           /* the acknowledge halts here */

	/*
	 * A line another card left asserted before this one was tickled
	 * is not this card's answer: the pending register is a level and
	 * not an event, so zero is not the only thing that can be in it.
	 * Wait for a bit to appear - a bit that clears is somebody
	 * else's business, and this card lowers nothing itself.
	 */
	before = pic_irr();
	djstart();

	for (n = 0; n < PROBEWAIT && (pic_irr() & ~before) == 0; n++)
		;
	return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
