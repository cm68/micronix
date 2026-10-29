/*
 * The block trampolines, in the u page.
 *
 * These four are the block switch's only door into a module.  They are
 * stateless - the entries they call live in ovlbvec[] and the segment in
 * bdevseg[] - so they ride the u page with consts.o and bus.o instead of
 * taking resident text.  Every call is a syscall or a strategy, never an
 * interrupt.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/ovl.h>
#include <errno.h>

extern unsigned bmajor();
extern int nodev(), iodone();
extern struct biovec ovlbvec[];

/*
 * The block trampolines.  Each recovers its major from the device it was
 * handed, which every call site passes (bopen, bclose, strat and bioctl
 * in sys/uio.c), and that is why four of them serve every major rather
 * than four for each.  A major with no driver behind it answers the way
 * the table's nodev row does, so an unloaded disk is indistinguishable
 * from one that was never there - except that a request against it
 * completes with an error rather than hanging, which is what a major
 * that may be loaded later has to do.
 *
 * The entry in the table is what says whether a driver is there, not the
 * segment map: ovlbmap does whatever mapping there is to do, and its
 * answer is not a test of presence.  A driver the kernel still holds has
 * entries and no segment, and then there is nothing to map.
 */
int
ovlopen(dev, mode)
    UINT dev;
    UINT8 mode;
{
    UINT8 maj;

    maj = bmajor(dev);
    ovlbmap(maj);
    if (ovlbvec[maj].open == 0) {
        nodev();
        return (0);
    }
    return ((*ovlbvec[maj].open) (dev, mode));
}

int
ovlclose(dev, mode)
    UINT dev;
    UINT8 mode;
{
    UINT8 maj;

    maj = bmajor(dev);
    ovlbmap(maj);
    if (ovlbvec[maj].close == 0) {
        nodev();
        return (0);
    }
    return ((*ovlbvec[maj].close) (dev, mode));
}

int
ovlstrat(b)
    register struct buf *b;
{
    UINT8 maj;

    maj = bmajor(b->dev);
    ovlbmap(maj);
    if (ovlbvec[maj].strat == 0) {
        b->flags |= BERROR;
        b->error = ENXIO;
        iodone(b);
        return (0);
    }
    return ((*ovlbvec[maj].strat) (b));
}

/*
 * Run a command block against a module's device (sys/uio.c's bioctl, which
 * has already copied the request in and found the block it moves the data
 * through).  The answer is the driver's, except for a driver that
 * declared no command block at all: that is not a failure of the request
 * but of the request against this device, and ENOTTY says so the way the
 * character side's mode() has always said it.
 */
int
ovlioctl(dev, cmd, r, b)
    UINT dev, cmd;
    struct cdb *r;
    struct buf *b;
{
    UINT8 maj;

    maj = bmajor(dev);
    ovlbmap(maj);
    if (ovlbvec[maj].ioctl == 0) {
        u.error = ENOTTY;
        return (-1);
    }
    return ((*ovlbvec[maj].ioctl) (dev, cmd, r, b));
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
