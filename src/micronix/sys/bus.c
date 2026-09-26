/*
 * The DMA bus lock, shared by the disk drivers.
 *
 * sys/bus.c
 *
 * Two controllers on the S-100 bus drive it themselves: the HD-DMA, the
 * hard disk controller (sys/mw.c), and the DJ-DMA, the floppy controller
 * (sys/dj.c).  Both are bus masters for the length of a transfer, and
 * the HD's transfer is long enough that letting the floppy start one in
 * the middle of it would close the hard disk's window.  So a driver that
 * wants the bus for a transfer asks for it (busget) and gives it back
 * when the transfer is over (busgive).
 *
 * Holding it is not the only answer.  A driver that asks while another
 * holds the bus is remembered as the heir - next below - and called when
 * the holder lets go, rather than being turned away and made to poll.
 * Neither driver knows the other exists, or that this file does: what
 * busget is given is a function to call, and what busgive returns is
 * whether anyone wanted it.
 *
 * Why this is not in mw.c, where it was.  mw and dj are modules now
 * (sys/OVERLAY-DRIVERS.md): each is linked on its own against the
 * kernel, so a function one of them defines is one the other cannot
 * reach, and a link of dj.mod naming busgive failed for exactly that
 * reason.  The u page is where the kernel keeps code two modules must
 * share - the leaf code and the libccc runtime are there for the same
 * reason (LEAF_C, sys/GNUmakefile) - so the lock is there, and both
 * modules resolve it out of the kernel's symbol table with -Aunix.
 *
 * What that costs.  The u page is per-process: newmap() remaps its
 * segment for each process and fork's bankcopy() clones the whole of it
 * (sys/user.c), so bus and next are a pair per process and not a pair
 * for the machine.  That is not the loss it looks like.  A transfer
 * holds the lock from its strategy routine to its completion, and the
 * buffer window (sys/mem.s) admits one transfer at a time, so the only
 * process that can be between a busget and its busgive is the one doing
 * the transfer - and the tick that calls mwcheck (sys/mw.c) to give the
 * bus back runs in whatever process it interrupted, which is that same
 * one.
 *
 * di()/ei(): the lock is taken and given from a strategy routine that an
 * interrupt can land in, and from the tick, and the two must not see a
 * half-updated bus/next.
 */
#include <types.h>

static int (*bus)() = 0;        /* bus master */
static int (*next)() = 0;       /* bus heir */

/*
 * Take the bus for func, or register it as the heir if someone else has
 * it.  Returns whether the bus was got.
 */
busget(func)
    int *(func) ();
{
    di();
    if (bus != 0 && bus != func) {   /* someone else has it */
        if (next == 0)       /* register the heir */
            next = func;
        ei();
        return 0;              /* didn't get it */
    } else {
        bus = func;             /* application accepted */
        ei();
        return 1;             /* got it */
    }
}

/*
 * Give the bus up, to the heir if there is one.  Returns whether it was
 * taken; a caller that gets 1 has been replaced and should stop driving
 * the bus, because the heir was called from here and is using it now.
 */
busgive(func)
    int *(func) ();
{
    di();
    if (next) {                 /* there is an heir */
        bus = next;             /* give bus to heir */
        next = func;            /* register next heir */
        ei();
        (*bus) ();              /* invoke new master */
        return 1;             /* bus was given away */
    } else {
        bus = 0;             /* no bus master */
        ei();
        return 0;              /* no one took bus */
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
