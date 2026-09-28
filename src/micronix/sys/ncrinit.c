/*
 * NCR 5380 SCSI host adapter: initialization, which here is the presence
 * probe and nothing else.
 *
 * sys/ncrinit.c
 *
 * This is the driver's init() (sys/ovl.h), the one entry a module hands
 * the kernel in its header.  It runs once, from ovlplace with the module's
 * page already mapped (ovlattach, sys/ovl.c), and it is folded into
 * highmem.o - the init-only region the linker parks after bss and
 * expand_bufs() reclaims as buffer headers once init has returned - so it
 * costs the running kernel nothing and, more to the point, takes nothing
 * out of the driver's one 4K page.
 *
 * What it does is answer whether the card is in the machine, because a
 * non-zero return is what ovlplace takes as exactly that: the driver is
 * not registered and major 5 goes on answering as nodev does.  The probe
 * is one register write and one read, and include/sys/ncr.h carries the
 * port, the register and the reasoning - the short of it is that a port
 * nothing answers reads 0xff, and the command register is the cpu's own
 * latch and so reads back what was written where the bus status register
 * could not.
 *
 * There is nothing else to switch on.  The chip's reset belongs at open
 * (ncropen calls scsireset), where it also serves a machine that came up
 * with the bus in a strange state; the interrupt line is already unmasked
 * by ARMMASTER in inits.s; and the driver reaches its card through I/O
 * ports alone, so no map register and no DMA channel has to be set up.
 *
 * A resident init cannot name anything in the module - not a function and
 * not a data object (sys/ovl.h) - and this one does not need to.  djinit
 * has to hand its board the address of a command stream out of the
 * driver's own segment and reaches it through ovldata(); this card is
 * driven by register writes, and the one address it needs (its own, in
 * boards[]) is the driver's business.  So nothing here goes near
 * ovldata(), and seg - the page the module was placed in - goes unused.
 * The parameter is the interface's, not this file's.
 */
#include <types.h>
#include <sys/ncr.h>

extern int in(), out();

ncrinit(seg)
    UINT seg;
{
    /*
     * Leave the card with every command line deasserted: RST, ACK, BSY,
     * SEL, ATN and the data drivers all off, which is a bus at rest and
     * the state a target expects to be selected from.
     */
    out(SCSIBASE + ICR, 0);

    /*
     * The six writable bits read back clear on a card that is there, and
     * a port with nothing behind it reads 0xff - every one of them set.
     */
    if (in(SCSIBASE + ICR) & ICR_WR)
        return (1);             /* nothing answered: no card here */
    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
