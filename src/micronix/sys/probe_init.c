/*
 * Interrupt probe and attach, init half.
 *
 * sys/probe_init.c
 *
 * intrset() and intr_attach() run once at boot, after every driver is
 * placed and before the first open, so this whole file folds into
 * highmem.o and is reclaimed as buffer headers.  The resident dispatch
 * (ovlint[], ovlntr) is in probe.c.
 *
 * A driver no longer names its line: its header (sys/ovl.h) carries a
 * probe instead of a line, and intr_attach discovers the line by
 * tickling each card in turn and reading the PIC's pending register.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/ovl.h>
#include <sys/pic.h>

struct ovlintr {
    int (*fn) ();
    UINT seg;
};

extern struct ovlintr ovlint[NOVLINE * OVLSHARE];  /* sys/probe.c */

/*
 * The pending attach list: each driver's probe, handler and segment,
 * taken as it is placed and resolved by intr_attach once every driver is
 * in.  The probe and handler live in the driver's module, so the segment
 * travels with them.  nintrdone is how far intr_attach has got, which is
 * what lets it run twice - see the note on it below.
 */
#define NPINTR  12              /* the module pages all carry a probe: nine
                                 * of them, and a stamped root driver too */
static int (*intrprobe[NPINTR]) ();
static int (*intrfn[NPINTR]) ();
static UINT intrseg[NPINTR];
static UINT8 nintr;
static UINT8 nintrdone;

/*
 * Claim a driver's interrupt for the attach phase.  ovl.c's ovlplace()
 * calls this with what the header declares - the probe (the tickle) and
 * the handler - plus the segment the driver was placed in.  Nothing is
 * wired yet: the line is not known until the card is tickled.
 */
intrset(probe, fn, seg)
    int (*probe) ();
    int (*fn) ();
    UINT seg;
{
    if (nintr >= NPINTR)
        return (-1);
    intrprobe[nintr] = probe;
    intrfn[nintr] = fn;
    intrseg[nintr] = seg;
    nintr++;
    return (0);
}

/*
 * The lowest line set in a mask, or -1 for none.  pic_irr() returns a
 * mask; with no sharing a tickle sets exactly one bit, and this names it.
 * Walk the bits low to high by shifting the mask rather than a 1 bit -
 * c1 has no rule for shifting by a register variable, so (1 << l) would
 * emit nothing.
 */
static
bitline(mask)
    int mask;
{
    register int l;

    for (l = 0; l < NOVLINE; l++) {
        if (mask & 1)
            return (l);
        mask >>= 1;
    }
    return (-1);
}

/*
 * The attach loop.  For each driver that declared an interrupt:
 *
 *      intoff(all)      the tickle must not reach the CPU
 *      before = irr     what was already standing
 *      probe()          the card asserts its line
 *      line = irr-before  the line this probe raised
 *      ovlint[line] = the handler
 *      pic_eoi(line)    clear the tickled interrupt
 *      inton(line)      the line is live from here on
 *
 * The baseline is not a nicety.  A card holds its VI line until the next
 * command, and a pending request is cleared by an acknowledge - not by an
 * EOI, which only clears the in-service bit.  While the lines are masked
 * there is no acknowledge, so every tickled line stays pending and the
 * next probe would find the earlier card's bit, wire its own handler onto
 * that line, and lose its own.  Taking the difference names the line this
 * probe raised and nothing else.
 *
 * A card that is not in the machine never asserts its line, so its probe
 * finds nothing and it is left unwired - the tickle doubles as the
 * presence test.
 *
 * It runs twice, and it has to.  The root device's driver is placed by
 * ovlstart() from cus(), and bopen(rootdev) follows it in main() - long
 * before ovlplaceall() can read the other modules out of the kernel's own
 * file, which is what the file is read through.  So cus() attaches what
 * is placed there (the root driver alone) and main() attaches the rest.
 *
 * Running twice is what nintrdone is for.  The second pass takes only the
 * drivers placed since the first - a redone tickle would be a second
 * interrupt nothing had asked for - and the lines are opened from
 * ovlint[] rather than from what was just found, because the masking
 * above is machine-wide and would otherwise close the lines the first
 * pass opened.  A line is live exactly when it has a handler.
 */
intr_attach()
{
    register UINT i, slot;
    register int l;
    int line, before;

    for (l = 0; l < NOVLINE; l++)
        intoff(l);              /* mask every line, whatever came before */

    for (i = nintrdone; i < nintr; i++) {
        before = pic_irr();     /* requests already standing */
        ovlcall(intrprobe[i], intrseg[i]);      /* the tickle */
        line = bitline(pic_irr() & ~before);    /* the line it raised */
        if (line < 0)
            continue;           /* no card there, nothing to wire */
        /* the first free slot on the line: two cards share a line and
         * each gets its own, the HD-DMA then the HDCA on VI0 */
        for (slot = 0; slot < OVLSHARE; slot++)
            if (ovlint[line * OVLSHARE + slot].fn == 0)
                break;
        if (slot < OVLSHARE) {
            ovlint[line * OVLSHARE + slot].fn = intrfn[i];
            ovlint[line * OVLSHARE + slot].seg = intrseg[i];
        }
        pic_eoi(line);          /* clear the tickled interrupt */
    }
    nintrdone = nintr;

    for (l = 0; l < NOVLINE; l++)
        for (slot = 0; slot < OVLSHARE; slot++)
            if (ovlint[l * OVLSHARE + slot].fn)
                inton(l);       /* the line is live */
    return (0);
}
