/*
 * Disk Jockey DMA: initialization.
 *
 * sys/djinit.c
 *
 * This is the driver's initialization split away from the driver itself
 * (sys/dj.c).  It runs once, from cus() (sys/cus.c) inside the di()/ei()
 * window the board's bring-up holds, and never again - so it is folded
 * into highmem.o, the init-only region the linker parks after bss and
 * expand_bufs() reclaims as buffer headers once init has returned.  It
 * costs the running kernel nothing, which is what keeps dj's own object
 * inside one 4K page: djinit was 407 bytes of the 4154 that did not fit.
 *
 * The driver's code and data stay together in the module: one page,
 * linked at OVLBASE and mapped at OVLSEG.  What this file may not do is
 * name any of that data.  Resident code cannot name a module symbol -
 * neither a function nor a data object - and knowing the segment a
 * module was placed in gives it a page, not an offset into one.  So the
 * driver's data is asked for rather than named, through the hook the
 * header's data field feeds (sys/ovl.c); what comes back is the offset
 * into the page that the driver's own link chose, in whatever segment
 * the caller placed the page.  While the driver is still linked into the
 * kernel there is no segment and the address is the kernel's own - the
 * same code, two placements.
 *
 * Everything else the setup below reaches - the controller's ports, the
 * map registers - is in the kernel, and stays reachable after this
 * object is gone.  What the driver *is* - its entries, its interrupt
 * line, its name, its tick - is not here either: the driver declares all
 * of that in its own header (sys/ovl.h), and the kernel takes it there.
 * An init that runs from the kernel has no way to say djopen, and this
 * one does not have to.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/dj.h>
#include <sys/ovl.h>

extern UINT8 image0[], map0[];
extern int out(), inton();

djinit(seg)
    UINT seg;
{
    static char *p, i;
    char *d;
    unsigned chan;

    /*
     * The command block is in the driver's own segment, and this code is
     * resident: it can name nothing in a module, a data object no more
     * than a function.  So it asks for it (sys/ovl.c), and what it is
     * given is an address in the driver's page, mapped for the moment.
     * The board is what follows the address written into the list below,
     * so that one has to be physical: the segment the page was placed in,
     * and the offset into the page the driver's own link chose.  Nothing
     * here looks inside the block.
     */
    d = ovldata(2);

    if (seg == 0)
        chan = (unsigned) d;
    else
        chan = (unsigned) ((seg << 12) | ((unsigned) d & 0xfff));

    p = DEFCHAN + 0x1000;

    map0[2] = 0;                /* contort the map */

    *p++ = SERIAL;              /* disable the serial port */
    *p++ = DISABLE;

    *p++ = LOGICAL;             /* set to 8" drives first */
    *p++ = EIGHTFIRST;
    *p++ = NOSTAT;

    *p++ = SETCHANNEL;          /* set the command address */
    *p++ = chan >> 0;
    *p++ = chan >> 8;
    *p++ = chan >> 16;

    for (i = 0; i < NDRIVES; i++) {
        *p++ = SETTRACK;
        *p++ = i;
        *p++ = MAXTRACK;
        *p++ = NOSTAT;
    }

    *p++ = HALT;
    *p++ = NOSTAT;

    map0[2] = image0[2];        /* restore the map */

    out(START, 0);

    inton(DJINT);

    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
