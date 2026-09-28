/*
 * block i/o 
 *
 * sys/uio.c 
 * Changed: <>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <errno.h>

extern long seconds;            /* see clock.c */
extern UINT nbuf;                /* initialized in binit(), main.c */
extern struct buf *btop;            /* ditto */
extern UINT8 map0[], image0[];   /* MMU map registers (uhdr.s) */
extern int copy();               /* leaf mem.s: kernel-to-kernel copy */
extern int copyin(), copyout();  /* leaf mem.s: the user side of it */

/*
 * Hash the (dev, blk) pair into a bucket.  Consecutive block numbers have
 * consecutive low bits, so blk's low bits spread evenly; dev is folded in
 * so two block devices don't always collide.
 */
#define NHASH   64

static struct buf *bhash[NHASH];

static int
hash(blk, dev)
    UINT blk, dev;
{
    return ((blk ^ dev) & (NHASH - 1));
}

/*
 * Remove a buffer from its hash bucket, called before its blk/dev change.
 */
static void
unhash(b)
    struct buf *b;
{
    struct buf **pp;
    int h;

    h = hash(b->blk, b->dev);
    for (pp = &bhash[h]; *pp; pp = &(*pp)->b_hash) {
        if (*pp == b) {
            *pp = b->b_hash;
            b->b_hash = 0;
            return;
        }
    }
}

/*
 * Get a buffer for the block
 */
struct buf *
bget(blk, dev)
    UINT blk, dev;
{
    register struct buf *b, *f;
    int h;

    h = hash(blk, dev);

  loop:
    /*
     * search the hash bucket for the block
     */
    for (b = bhash[h]; b; b = b->b_hash) {
        if (b->blk == blk && b->dev == dev) {
            if (block(b))
                return (b);
            else
                goto loop;      /* busy: retry */
        }
    }
    /*
     * block not found: pick the least-recently-used free buffer
     */
    f = 0;
    for (b = blist; b < btop; b++) {
        if (b->flags & (BBUSY | BLOCK))
            continue;
        if (f == 0 || b->time < f->time)
            f = b;
    }
    if ((b = f) == 0)        /* no available buffers */
        goto loop;
    if (!block(b))           /* raced: someone else took it, retry */
        goto loop;
    if (b->flags & BDELWRI) {
        bwrite(b);
        goto loop;
    }
    unhash(b);               /* drop it from its old blk's bucket */
    sbrel(b);                /* and from a superblock slot, if it had one */
    b->blk = blk;
    b->dev = dev;
    b->flags &= ~BDONE;
    b->b_hash = bhash[h];
    bhash[h] = b;

    /*
     * zero(b->data, 512);
     */
    return (b);
}

/*
 * Lock a block if possible.
 */
block(b)
    struct buf *b;
{
    di();
    if (b->flags & BBUSY) {
        b->flags |= BWANT;
        sleep(b, PRIBIO);
        return 0;
    }
    b->flags |= BBUSY;
    ei();
    return 1;
}

/*
 * Read the indicated block (if necessary)
 */
struct buf *
bread(blk, dev)
    UINT blk, dev;
{
    register struct buf *b;

    b = bget(blk, dev);

    if ((b->flags & (BDONE | BERROR)) != BDONE) {
        b->flags |= BREAD | BSYNC;
        strat(b);
        bwait(b);
    }

    if (geterror(b)) {
        brelse(b);
        return 0;
    } else {
        return (b);
    }
}

/*
 * Read a block asyncronously. Used for read-ahead by iread (fio.c)
 */
aread(blk, dev)
    UINT blk, dev;
{
    register struct buf *b;

    for (b = blist; b < btop; b++)
        if (b->blk == blk && b->dev == dev)
            return;

    b = bget(blk, dev);
    b->flags &= ~BSYNC;
    b->flags |= BREAD;
    strat(b);
}

/*
 * The superblock's home in the kernel's own address space.
 *
 * It used to be read through the 0xd000 window, held there across calls
 * that map other buffers and across sleeps.  A held window is exactly what
 * the buffer pool cannot have: that page now carries buffer headers, and
 * whoever holds the window has to reach headers between mapping it and
 * restoring it.  So each device's superblock gets a slot of plain memory
 * instead, and its buffer's data/xmem are pointed at it.  That keeps
 * sb->data an ordinary pointer - no mapping, nothing to re-pin after a
 * sleep - and the driver still writes the right bytes, because its DMA
 * address is (xmem << 12) | (data & 0xfff), not the window address.
 *
 * The mapping to restore is saved here because the buffer's data/xmem are
 * assigned once by binit()/expand_bufs() and bzero() preserves them: on
 * umount the buffer goes back into the pool and must get its window
 * mapping back (sbrel).
 */
struct sbslot {
    struct buf *b;              /* the buffer this slot belongs to, 0 = free */
    char *wdata;                /* that buffer's window address, saved */
    UINT8 wxmem;                /* and its segment */
    char data[512];             /* the superblock, a block like any other */
};

struct sbslot sbslot[NMOUNT];

/*
 * Read the super-block (block 1) on the device.
 *
 * bread() leaves the block in the buffer's own segment; the first call for
 * a device copies it into a slot and points the buffer there.  Later calls
 * find it already attached - the copy must not be repeated then, because
 * the segment copy is stale the moment the superblock is modified.
 */
struct buf *
getsb(dev)
    UINT dev;
{
    register struct buf *b;
    register struct sbslot *s, *f;

    if ((b = bread(1, dev)) == 0)
        panic("cant get superblock");   /* should be locked in core */

    f = 0;
    for (s = sbslot; s < sbslot + NMOUNT; s++) {
        if (s->b == b)
            return (b);                 /* already attached */
        if (s->b == 0 && f == 0)
            f = s;
    }
    if (f == 0)
        panic("too many superblocks");
    di();
    copy(bhold(b), f->data, 512);
    brel();
    ei();
    f->wdata = b->data;
    f->wxmem = b->xmem;
    b->data = f->data;
    b->xmem = (UINT8)((UINT)f->data >> 12);
    f->b = b;
    return (b);
}

/*
 * Give the superblock buffer its window mapping back and free its slot.
 * Called when the device is unmounted: the buffer returns to the pool, and
 * its data may not still be pointing at the slot.
 */
sbrel(sb)
    struct buf *sb;
{
    register struct sbslot *s;

    for (s = sbslot; s < sbslot + NMOUNT; s++)
        if (s->b == sb) {
            sb->data = s->wdata;
            sb->xmem = s->wxmem;
            s->b = 0;
            return;
        }
}

/*
 * Write out the buffer synchronously and release it. Geterror
 * could be included here, but so far nothing needs it, and it
 * would complicate some routines where the current user is not
 * responsible for the write (such as bget above). The calling
 * code can still look at the error (even though the buffer has
 * been released) since process switching cannot occurr inside
 * the kernel except across sleep.
 */
bwrite(b)
    register struct buf *b;
{
    b->flags |= BSYNC;
    b->flags &= ~BREAD;
    strat(b);
    bwait(b);
    brelse(b);
}

/*
 * Write out the buffer asynchronously.
 * There is no error reporting, except
 * device messages to console. Iodone
 * will release the buffer.
 */
bawrite(b)
    struct buf *b;
{
    b->flags &= ~(BSYNC | BREAD);
    strat(b);
}

/*
 * Mark the buffer for later writeout and release it.
 */
bdwrite(b)
    struct buf *b;
{
    b->flags |= (BDELWRI | BDONE);
    brelse(b);
}

/*
 * Release the buffer
 */
brelse(b)
    register struct buf *b;
{
    if (b == 0 || (b->flags & BBUSY) == 0)
        return;
    if (b->flags & BWANT)
        wakeup(b);
    b->flags &= ~(BBUSY | BWANT);
    b->time = btime();
}

/*
 * Increment a count and return it as a reference "time".
 * Called from brelse. When the count recycles, reset all
 * buffer times to zero.
 */
unsigned
btime()
{
    static UINT count = 0;
    static struct buf *b;

    di();
    if (++count == 0) {
        for (b = blist; b < btop; b++)
            b->time = 0;
    }
    ei();
    return (count);
}

/*
 * Report the max block io queue length. Debugging.
 */
char quelen = 0;

/*
 * Sort a queue of block I/O requests into a reasonable order.
 * For use by strategy routines. Must be di()'ed.
 */
bsort(h, b)
    struct buf *h, *b;
{
    static struct buf *p, *f;
    static UINT bc, pc, fc;
    static char n;

    bc = b->cyl;
    n = 1;
    for (p = h; f = p->forw; p = f) {
        n++;
        pc = p->cyl;
        fc = f->cyl;
        if ((pc <= bc && bc <= fc) || (pc >= bc && bc >= fc))
            break;
    }
    if (n > quelen)
        quelen = n;
    b->forw = f;
    p->forw = b;
}

/*
 * Access the strategy routine to read or write a buffer
 */
strat(bp)
    struct buf *bp;
{
    static struct buf *b;

    b = bp;
    b->flags |= BBUSY;
    b->flags &= ~(BDONE | BERROR);
    b->error = 0;
    b->count = 512;
    b->forw = 0;
    b->back = 0;
    (*biosw[bmajor(b->dev)].strat) (b);
}

/*
 * Access a block device open routine.
 */
bopen(dev, mode)
    UINT dev, mode;
{
    (*biosw[bmajor(dev)].open) (dev, mode);
}

/*
 * Access a block device close routine.
 */
bclose(dev, mode)
    UINT dev, mode;
{
    (*biosw[bmajor(dev)].close) (dev, mode);
}

/*
 * Run a raw command block against a block device (include/sys/ioctl.h).
 *
 * The caller names the device the way read and write do, with a
 * descriptor, and the device number comes off the inode the file is open
 * on - ip->i_addr[0], which is where an io node keeps its device, and the
 * same field iread finds its breads' dev in (fio.c).  A descriptor that
 * is not open, or is not a block device, is refused here; whether the
 * command suits the device is the driver's business below.
 *
 * The rest of the caller's structure says which bytes are the block,
 * which way the data goes, and where it lands.  The block itself is never
 * looked into: only the driver's bus has an opinion about what a CDB or a
 * task file means, and that is the whole point of the call.
 *
 * What the kernel owes the driver is a place for the data the window can
 * hold - a buffer from the cache.  That is the same one-block object a
 * strategy routine is handed, so it is the only shape a driver already
 * knows how to move, and the ioctl entry is written against it for
 * exactly that reason.  It comes out of the pool for the length of the
 * call and goes back, so a burst of command blocks costs the cache
 * nothing lasting.
 *
 * A data phase past that one block gets no buffer and is the driver's to
 * move, since the kernel has no second shape to move it in and the driver
 * has a page of its own or can get one.  Up to CDBDATA, which is a page,
 * because a driver that stages is staging into one.
 *
 * The buffer is not any block of any device, and that is worth saying
 * out loud: its identity is (0, 0), and major 0 is nodev, so nothing can
 * ever bread a block with the same key and be handed this.  A scratch
 * header that aliased a real block would hand a filesystem this data and
 * call it a disk read.
 *
 * The copies around the driver are the read and write paths' own
 * (fio.c): a hold on the buffer's window with copyin/copyout through it.
 * BUFSEG is the window that may be held across those copies.  The
 * scratch window may not, because mem.s borrows that one for the copies
 * themselves.
 */
int
bioctl(fd, cmd, arg)
    UINT fd, cmd;
    char *arg;
{
    struct cdb r;
    register struct buf *b;
    register struct file *fp;
    register struct inode *ip;
    UINT dev;
    int maj, err;

    if (cmd != CDBCMD) {
        u.error = EINVAL;
        return (-1);
    }
    if ((fp = ofile(fd)) == 0)
        return (-1);            /* ofile set u.error = EBADF */
    ip = fp->inode;
    if ((ip->i_mode & IFMT) != IFBLK) {
        u.error = ENOTTY;
        return (-1);
    }
    dev = ip->i_addr[0];
    maj = bmajor(dev);
    if (biosw[maj].ioctl == 0) {
        u.error = ENOTTY;
        return (-1);
    }

    copyin(arg, (char *) &r, sizeof r);
    if (r.len < 1 || r.len > CDBMAX || r.count < 0 || r.count > CDBDATA) {
        u.error = EINVAL;
        return (-1);
    }

    /*
     * A data phase that fits in a block is one the kernel moves, and the
     * driver is handed the block.  A bigger one it moves itself: no block
     * is taken, b is zero, and r.buf is left for the driver to reach
     * through a page of its own.  The driver can tell the two apart from
     * b, and r.count says how much there is either way.
     */
    b = 0;
    if (r.count && r.count <= 512) {
        b = bget(0, 0);
        b->count = r.count;
        if ((r.flags & CDB_IN) == 0) {
            copyin(r.buf, bhold(b), r.count);
            brel();
        }
    }

    err = (*biosw[maj].ioctl) (dev, cmd, &r, b);

    if (err == 0 && b && r.count && (r.flags & CDB_IN)) {
        copyout(bhold(b), r.buf, r.count);
        brel();
    }
    if (b)
        brelse(b);

    /*
     * The block goes back to the caller as it came, except for what the
     * driver wrote in it: a bus that answers a command inside the command
     * block - the DJDMA leaves a status byte in each command it runs -
     * has no other way to be heard, and this costs a caller who set
     * nothing the same bytes it sent.
     */
    copyout((char *) &r, arg, sizeof r);

    if (err) {
        if (u.error == 0)
            u.error = EIO;
        return (-1);
    }
    return (0);
}

/*
 * Return the major device number.
 */
unsigned
bmajor(dev)
    UINT dev;
{
    static UINT maj;

    if ((maj = dev >> 8) >= nbdev)
        maj = 0;                /* nodev, sets b->error = ENXIO */
    return (maj);
}

/*
 * Called at io completion time. Adjust flags.
 * If io was synchronous, issue wakeup.
 * If io was asynchronous, release buffer.
 */
iodone(bp)
    struct buf *bp;
{
    static struct buf *b;
    static UINT dev;

    b = bp;
    dev = b->dev;
    b->flags |= BDONE;
    b->flags &= ~BDELWRI;
    if (b->flags & BERROR)
        perror(b);
    if (b->flags & BSYNC)
        wakeup(b);
    else
        brelse(b);              /* brelse handles wakeup on BWANT */
}

/*
 * Print an I/O error message at the console
 */
perror(b)
    struct buf *b;
{
    if (b->error == ENXIO || (b->flags & (BREAD | BSYNC)) == BREAD)
        return;
    pr((b->flags & BREAD) ? "Read" : "Write");
    pr(" error block %i", b->blk);
    prdev("", b->dev);
}

/*
 * Wait for io completion on buffer
 */
bwait(b)
    struct buf *b;
{
    for (;;) {
        di();
        if (b->flags & BDONE)
            break;
        b->flags |= BWANT;
        sleep(b, PRIBIO);
    }
    ei();
}

/*
 * Set u.error from the buffer
 */
geterror(b)
    struct buf *b;
{
    return (u.error = (b->flags & BERROR) ? ((b->error) ? b->error : EIO)
        : 0);
}

/*
 * Flush dev's blocks from the blist. Called from iclose.
 * Note that iclose holds the inode, so there is no
 * danger that anyone else will access the device while
 * this is going on (as long as there is only one inode).
 */
bflush(dev)
    UINT dev;
{
    register struct buf *b;

  loop:
    for (b = blist; b < btop; b++)
        if (dev == b->dev) {
            if (!block(b))
                goto loop;
            if (b->flags & BDELWRI)
                bwrite(b);
            else
                brelse(b);
            unhash(b);
            bzero(b);
        }
}

/*
 * Zero a buffer, taking it out of the cache first.
 *
 * unhash() has to come before the zero: it finds a buffer by hashing its
 * blk and dev, so once those are cleared the buffer can never be unhooked
 * again.  It would stay in its bucket while bget() handed it out for other
 * blocks, linking it into a second bucket and closing a loop in the chain
 * - and the lookup walk in bget() only ends at a null link, so it would
 * spin there forever.  A driver that drops a buffer it read (mwclose) is
 * the case this guards.
 */
bzero(b)
    struct buf *b;
{
    char *data;
    UINT8 xmem;

    unhash(b);
    /* A buffer recycled by bflush() may still be a superblock's (getsb
     * pointed its data at a slot).  Detach it first - that is what puts
     * its window address and segment back - and then preserve them. */
    sbrel(b);
    data = b->data;
    xmem = b->xmem;
    zero(b, sizeof(*b));
    b->data = data;
    b->xmem = xmem;
}

/*
 * Sync the blist. Called from sync().
 */
bsync()
{
    register struct buf *b;
    register struct super *s;

    b = getsb(rootdev);
    s = (struct super *)b->data;

    if (!s->s_flock) {            /* not read-only */
        di();
        s->s_time = seconds;
        ei();
        bdwrite(b);
    } else
        brelse(b);

    for (b = blist; b < btop; b++)
        if ((b->flags & (BBUSY | BDELWRI)) == BDELWRI)
            bawrite(b);
}

/*
 * Count the buffers still carrying a delayed write - the length of the
 * dirty list.  A busy buffer is being written, not waiting, so only the
 * not-busy delayed-write buffers count, as in bsync().
 */
int
ndirty()
{
    register struct buf *b;
    register int n = 0;

    for (b = blist; b < btop; b++)
        if ((b->flags & (BBUSY | BDELWRI)) == BDELWRI)
            n++;
    return n;
}

/*
 * Write every delayed-write buffer out synchronously.  bwrite waits for
 * the write to finish, so across the reboot() loop the count really does
 * drain to zero rather than just being issued.
 */
int
bdrain()
{
    register struct buf *b;
    register int n = 0;

    for (b = blist; b < btop; b++)
        if ((b->flags & (BBUSY | BDELWRI)) == BDELWRI) {
            bwrite(b);
            n++;
        }
    return n;
}

/*
 * Map a buffer's segment into the BUFSEG window so its data (BUFWIN +
 * offset) becomes reachable.  b->xmem carries the segment; the window is
 * segment BUFSEG, so its map register is map0[2*BUFSEG] == map0[28].
 *
 * This is the raw mapping and it leaves the segment in place.  bhold()
 * below is what a buffer access uses: the same write, but the segment
 * that was mapped goes on a stack and comes back at brel().  Held that
 * way the window's page is the kernel's own memory whenever no hold is
 * up, which is what lets the page carry code - see TODO.
 *
 * The pool still stops below the window's page (BUFWIN, see
 * expand_bufs): a header there would be read back as buffer data for as
 * long as a hold is up, and holds are taken all over the filesystem.
 */
bwin(seg)
    int seg;
{
    map0[2 * BUFSEG] = image0[2 * BUFSEG] = seg;
}

/*
 * Hold the window on a buffer's data and return the address of its 512
 * bytes.  brel() gives the window back.
 *
 * Holds stack: the segment in place before one goes on a small stack and
 * brel() puts it back, so a callee's hold does not lose the caller's and
 * a hold taken in an interrupt does not lose the one it interrupted.
 * Since nothing puts a segment in this window except through here, the
 * window is the kernel's own segment whenever bholdn is 0 - the image
 * the firmware left it at, which is the page this kernel's code is in.
 *
 * The stack holds image0, not map0: the map registers are write only,
 * so reading one back is not the segment in place, it is bus garbage.
 * Saving map0 here and restoring it at brel() left the window mapped to
 * page 0 - the rom's page - after the first hold, which went unnoticed
 * while nothing but buffers was reached through that page.
 *
 * Two rules keep a hold short enough for that to be worth anything:
 *
 *   - Never sleep while holding one.  sleep() panics on it: the next
 *     process would run with this one's buffer over the window's page.
 *   - No header access (bget/brelse/bdwrite) and no call that maps the
 *     window for its own use while one is held.  The hold you get back
 *     is the one you took - imapb() is the shape: it drops its hold
 *     before plug(), which reaches balloc() and can sleep.
 *
 * bhold() returns b->data.  A superblock buffer is the exception: getsb()
 * moved its data to a slot in the kernel's own memory and its xmem
 * followed, so a hold on one maps a kernel segment where nothing needs
 * it.
 */
#define NHOLD   8

UINT8 bholdn;                   /* holds up: sleep() panics on a nonzero */
static UINT8 holdstk[NHOLD];

char *
bhold(b)
    register struct buf *b;
{
    if (bholdn >= NHOLD)
        panic("bhold: too deep");
    holdstk[bholdn++] = image0[2 * BUFSEG];
    map0[2 * BUFSEG] = image0[2 * BUFSEG] = b->xmem;
    return (b->data);
}

/*
 * Release the innermost hold, putting back the segment it saved.
 */
void
brel()
{
    if (bholdn == 0)
        panic("brel: no hold");
    map0[2 * BUFSEG] = image0[2 * BUFSEG] = holdstk[--bholdn];
}

/*
 * Hold the scratch window on a segment of the kernel's own choosing, and
 * return the address its bytes become reachable at.  srel() gives the
 * page back.
 *
 * This is bhold()/brel() on the other window, and it is here for a driver
 * that has to see a segment of its own *and* a buffer at the same time:
 * bhold() puts the buffer at BUFSEG, so the segment has to be reached
 * somewhere else, and SCRSEG is the one page left.  The two are otherwise
 * the same shape, down to saving image0 rather than map0 - the map
 * registers are write only, so reading one back is bus garbage and not
 * the segment in place.
 *
 * Sharing SCRSEG with mem.s's getbyte/putbyte/memrw, which borrow the
 * same register for the same page, is safe because those hold interrupts
 * off across the borrow (mem.s), so nothing can be entered in the middle
 * of one and find the register part way between two segments.  The rules
 * the caller keeps are the buffer window's - never sleep with one held,
 * and call nothing that maps this window while one is up - plus one that
 * follows from the sharing: a hold taken in a handler is fine, but a hold
 * taken in process context must not be held across anything that reaches
 * mem.s.
 */
#define NSHOLD  4               /* holds stack, as bhold's do */

static UINT8 sholdn;                    /* scratch holds up */
static UINT8 sholdstk[NSHOLD];

char *
swin(seg)
    int seg;
{
    if (sholdn >= NSHOLD)
        panic("swin: too deep");
    sholdstk[sholdn++] = image0[2 * SCRSEG];
    map0[2 * SCRSEG] = image0[2 * SCRSEG] = seg;
    return ((char *)SCRWIN);
}

/*
 * Release the innermost scratch hold, putting back the segment it saved.
 */
void
srel()
{
    if (sholdn == 0)
        panic("srel: no hold");
    map0[2 * SCRSEG] = image0[2 * SCRSEG] = sholdstk[--sholdn];
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
