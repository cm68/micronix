/*
 * The DMA bus lock, shared by the disk drivers.
 *
 * sys/bus.c
 *
 * Two controllers on the S-100 bus drive it themselves: the HD-DMA, the
 * hard disk controller (sys/mw.c), and the DJ-DMA, the floppy controller
 * (sys/dj.c).  Both are bus masters for the length of a transfer, and the
 * HD's transfer is one the bus cannot share: it seizes the bus and holds
 * it until a whole sector has been burst in.
 *
 * The bus has an arbiter - HOLD answered with HLDA, and the priority
 * daisy chain deciding which card asking for it is granted - and what the
 * arbiter cannot do is make that burst shareable.  Granting the bus to
 * the HD for the length of it leaves the DJ's requests unanswered, and
 * the DJ-DMA is unbuffered: there is no fifo holding bytes while it
 * waits, so the floppy controller's window passes underneath and the
 * sector is lost.  The arbiter serializes mastership.  It does not make
 * the floppy's timing survive being kept waiting.
 *
 * That is the reason this lock exists, in mw.c's own words, which dj.c
 * still points at ("can't get bus yet.  See mw.c"): "Since the HD must
 * hog the bus, it might close some of the DJ's transfer windows if they
 * were allowed to be active simultaneously.  Note that this code does not
 * know anything about the DJ drivers."
 *
 * So it is the DJ's transfer that suffers, not the HD's, and what the
 * lock buys is that the DJ never begins one it cannot finish: a request
 * that finds the bus held is registered as the heir and starts when the
 * HD is done, rather than starting and then being starved partway.  A
 * driver that wants the bus for a transfer asks for it (busget) and gives
 * it back when the transfer is over (busgive).
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
 * What that costs, and where the words are.  The u page is per-process:
 * newmap() remaps its segment for each process and fork's bankcopy()
 * clones the whole of it (sys/user.c).  Code in it is the same code in
 * every process, which is what makes it the place for code two modules
 * share, but mutable data in it is a different word per process, which
 * for this lock is fatal.  A transfer's busget to its busgive is not a
 * pair inside one process: both drivers sleep holding the bus - mw's
 * mwstop waits for its controller, dj's busplease sleeps for the heir to
 * call it - so another process runs in between.  A busgive run there
 * reads its own copy of the heir, finds 0, and drops the heir the
 * sleeping process registered: the heir is never called, the transfer it
 * was to start never starts, and nothing left in either process notices.
 * This is not a theory; it is what a stalled copy was, with mwstart the
 * heir that was dropped and the words at five different physical
 * addresses, one per process.
 *
 * So busmaster and busheir are not here.  They are in sys/ovl.c, in the
 * kernel's own data, beside the other state modules share and for the
 * same reason: one bus for the machine.  What is here is the code, in
 * the u page, because that is where a module finds a function it and
 * another module both call.
 *
 * di()/ei(): the lock is taken and given from a strategy routine that an
 * interrupt can land in, and from the tick, and the two must not see a
 * half-updated master and heir.
 *
 * Each pointer carries the segment it lives in.  The heir is a function
 * in another module as often as not - mw registers mwstart while dj
 * holds the bus - and an address in a module is an offset from OVLBASE
 * that means nothing unless that module's page is the one mapped.
 * Calling it with the current holder's page still in the window runs it
 * in the wrong segment: mwstart's offset lands in the middle of dj's
 * text, with dj's data under it, and what it does there is whatever the
 * bytes say.  So the heir goes through ovlcall (sys/ovl.c), and each
 * pointer is stored with the segment that was mapped when it was
 * registered - which is its own, since a module registers from inside
 * itself (ovlmap maps the major before the kernel calls into it).
 */
#include <types.h>
#include <sys/ovl.h>

extern UINT8 image0[];

extern int (*busmaster) ();	/* the words are in sys/ovl.c: one per machine */
extern UINT busmasterseg;
extern int (*busheir) ();
extern UINT busheirseg;

/*
 * Take the bus for func, or register it as the heir if someone else has
 * it.  Returns whether the bus was got.
 */
busget(func)
    int *(func) ();
{
    di();
    if (busmaster != 0 && busmaster != func) {  /* someone else has it */
        if (busheir == 0) {     /* register the heir */
            busheir = func;
            busheirseg = image0[2 * OVLSEG];
        }
        ei();
        return 0;              /* didn't get it */
    } else {
        busmaster = func;       /* application accepted */
        busmasterseg = image0[2 * OVLSEG];
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
    if (busheir) {              /* there is an heir */
        busmaster = busheir;    /* give bus to heir */
        busmasterseg = busheirseg;
        busheir = func;         /* register next heir */
        busheirseg = image0[2 * OVLSEG];
        ei();
        ovlcall(busmaster, busmasterseg);  /* invoke new master, in its page */
        return 1;             /* bus was given away */
    } else {
        busmaster = 0;          /* no bus master */
        ei();
        return 0;              /* no one took bus */
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
