/*
 * Interrupt dispatch, resident half.
 *
 * sys/probe.c
 *
 * The table the PIC's vector stubs land on, and the dispatch that walks
 * it.  Both run at every interrupt, so they are resident; the boot-time
 * attach (intrset, intr_attach) is in probe_init.c, folded into highmem
 * and reclaimed.
 *
 * A driver no longer names its line: its header (sys/ovl.h) carries a
 * probe instead of a line, and probe_init.c's intr_attach discovers the
 * line at boot by tickling each card in turn and reading the PIC's
 * pending register.  This table is what that attach fills, one entry per
 * line.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/ovl.h>
#include <sys/pic.h>
#include <sys/pic8259.h>

extern int in(), out();

/*
 * A handler on a line.  One card per line - a second card that tickles a
 * taken line is a wiring error, and the attach panics on it - so ovlint
 * is flat, one entry per line.  Sized to the widest board (MAXNOVLINE);
 * the boot-found nlines is how many of them are real.
 */
struct ovlintr {
    int (*fn) ();
    UINT seg;
};

struct ovlintr ovlint[MAXNOVLINE]; /* not static: probe_init.c's attach writes it */

/*
 * Dispatch a line's handler.  The PIC's vector stubs hand the line here
 * with nothing mapped; this puts the handler's module in place and calls
 * it.  A line nothing claimed returns without doing anything - the
 * interrupt from a card that never attached, or a spurious one.
 */
ovlntr(line)
    UINT line;
{
    if (line < nlines && ovlint[line].fn != 0)
        ovlcall(ovlint[line].fn, ovlint[line].seg);
    /*
     * Dismiss the line even when nothing claimed it.  The 8259 has
     * already acknowledged the request and put the line in service by
     * the time this runs; an early return leaves that bit set, and a
     * line in service blocks every lower-priority request - including
     * the cascade the clock arrives on.  A card that tickles its line
     * without a driver attached is then enough to stop time.
     */
    pic_eoi(line);
    return (0);
}

/*
 * The line a handler was wired to.  The attach knows it when it writes
 * ovlint[]; a driver that needs it again - the net driver, whose level
 * line must be masked when nobody reads - finds it here rather than
 * naming it, so the wiring stays discovered.  The address and the
 * segment are both compared: a module's functions are at
 * OVLBASE-relative addresses, so two modules can share one.  The
 * handler calls this from an interrupt, so it is a walk of at most
 * MAXNOVLINE entries.  -1 is not attached.
 */
int
intrline(fn, seg)
    int (*fn) ();
    UINT seg;
{
    register int l;

    for (l = 0; l < nlines; l++)
        if (ovlint[l].fn == fn && ovlint[l].seg == seg)
            return (l);
    return (-1);
}

/*
 * Mask a line at run time, the resident counterpart of inton().  Until
 * now only the boot attach had to close a line and it is init-only code,
 * so this was missing.  It is resident text, not u-page leaf code, so a
 * handler can call it with whatever u page the interrupted process has.
 *
 * No di()/ei(): the only caller is an interrupt handler, which runs with
 * interrupts already off.  A process-context caller would have to keep
 * its read-modify-write of the IMR from racing the handler's.
 */
int
pic_mask(line)
    UINT line;
{
    register struct pic8259 *p;

    p = &pics[linepic[line]];
    if (p->sel)
        out(p->sel, p->selval);
    out(p->icw1, in(p->icw1) | (1 << (lineir[line] & 7)));
    return (0);
}
