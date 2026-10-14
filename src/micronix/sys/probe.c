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

/*
 * A handler on a line.  No sharing: one controller, one line, so one
 * entry per line.  Flat, not a table of tables, because c0 cannot parse
 * a two-dimensional declarator.
 */
struct ovlintr {
    int (*fn) ();
    UINT seg;
};

struct ovlintr ovlint[NOVLINE]; /* not static: probe_init.c's attach writes it */

/*
 * Dispatch a line's handler.  The PIC's vector stubs hand the line here
 * with nothing mapped; this puts the handler's module in place and calls
 * it.  A line nothing claimed returns without doing anything - the
 * interrupt from a card that never attached, or a spurious one.
 */
ovlntr(line)
    UINT line;
{
    if (line >= NOVLINE)
        return (0);
    if (ovlint[line].fn)
        ovlcall(ovlint[line].fn, ovlint[line].seg);
    return (0);
}
