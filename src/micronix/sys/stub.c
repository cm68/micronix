/*
 * Stub block-device driver.
 *
 * sys/stub.c
 *
 * Every hook a driver has, and none of the code: open, close, strategy,
 * ioctl, init and tick are here as empty stubs; the interrupt handler is
 * in stub_intr.c; docs/STUB.md (this directory) is the recipe for turning this
 * into a real driver.
 *
 * This is a driver module (sys/ovl.h): its first byte is a struct ovlhdr,
 * linked at OVLBASE and copied into one 4K page at run time.  The kernel
 * learns the driver's entry points only from that header, so the header
 * is the interface - the kernel holds no driver symbol, and the driver
 * calls no kernel function.  A real driver puts the header in its own
 * object named first on the link line (idehdr.c is the example); it is
 * folded in here so the stub is self-contained, and docs/STUB.md says why and
 * when to split it out.
 *
 * The block-device switch is the four entries in a struct biovec
 * (open, close, strategy, ioctl).  The kernel calls them through the
 * resident switch (sys/consts.c), which is filled from the header when
 * the module is placed (sys/ovl.c, ovlattach).
 */

#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/ovl.h>

/*
 * The driver's private data.  A real driver keeps its board state here -
 * the controller's register addresses, the request in flight, the
 * geometry it read off the card - and names it in the header's data
 * field, because init() and the entry points are the only doors and a
 * resident init cannot name anything in the module except through that
 * one pointer (ovldata, sys/ovl.h).  Empty for now.
 */
struct stub {
    int open;                   /* something has opened the device */
    int busy;                   /* a request is in flight */
    int done;                   /* the interrupt said it is done */
};

/*
 * The interrupt handler, in stub_intr.c.
 */
extern int stubint();

stubopen(dev, mode)
    int dev;
    int mode;
{
    /*
     * open(dev, mode): something has named this device - a mount, or an
     * open of the raw device.  Probe the controller if that has not
     * happened, reset it, and take the hardware.  Return 0, or an errno.
     */
    return (0);
}

stubclose(dev)
    int dev;
{
    /*
     * close(dev): the last user is gone.  Release the hardware and undo
     * what open took.  Return 0, or an errno.
     */
    return (0);
}

stubstrat(b)
    register struct buf *b;
{
    /*
     * strat(b): do the I/O described by the buffer header.  The ordinary
     * shape is a filesystem request - one 512-byte sector, count fixed
     * at 512 by strat() - but swapio() can reach this routine directly
     * with a 4096-byte count; docs/STUB.md says what to refuse.
     *
     * The synchronous shape, the one ide.c uses:
     *   - check b->blk is inside the device and b->count is the shape
     *     this driver moves; on a bad request set BERROR and the errno
     *     and fall through to iodone;
     *   - start the transfer on the controller;
     *   - bhold(b) to get the buffer's bytes into the window;
     *   - move the sector in or out;
     *   - brel(); iodone(b).
     *
     * An interrupt-driven driver starts the transfer, sleeps on the
     * request's done word (sleep(&done, PRIBIO)), and lets stubint()
     * wake it; docs/STUB.md has both shapes.  The stub refuses everything.
     */
    b->flags |= BERROR;
    b->error = EIO;
    iodone(b);
}

stubioctl(dev, cmd, r, b)
    int dev;
    int cmd;
    int r;
    struct buf *b;
{
    /*
     * ioctl(dev, cmd, r, b): run the raw command block in b against the
     * device (sys/ioctl.h).  A driver whose bus has no command block
     * declares 0 in the switch and answers ENOTTY here, which is also the
     * stub's answer.
     */
    return (ENOTTY);
}

stubtick()
{
    /*
     * tick(): once a second, with the module's page mapped, for a driver
     * that must go and look at a controller that has stopped talking
     * (a timed-out request, a reset).  Most drivers have none and leave
     * the header's tick field 0; if this one is kept, name it there.
     */
}

/*
 * init(seg): the one entry the header hands the kernel, run once at
 * placement with the module's page already mapped, before the driver is
 * reachable.  It probes for the card and switches it on; return 0 to
 * register the driver, or non-zero to say the hardware is not in this
 * machine, in which case the major keeps answering as nodev.  ncrinit.c
 * is the presence-probe example.
 */
stubinit(seg)
    UINT seg;
{
    return (0);
}

/*
 * The block switch for this major.
 */
struct biovec stubbvec = {
    &stubopen, &stubclose, &stubstrat, &stubioctl
};

/*
 * The header (sys/ovl.h).  Every field is a placeholder that docs/STUB.md
 * tells how to fill in: major 0 is reserved for nodev, line 0 is the
 * hard-disk line, and data 0 means no private structure yet.
 */
struct ovlhdr stubhdr = {
    0,                          /* major */
    &stubinit,                  /* init */
    0,                          /* tick - 0 for none */
    0,                          /* data - the struct stub pointer */
    &stubbvec,                  /* bvec */
    0,                          /* cvec - a block driver has none */
    0,                          /* line - the interrupt line */
    &stubint,                   /* intr */
    { 's', 't', 'u', 'b' },     /* name */
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
