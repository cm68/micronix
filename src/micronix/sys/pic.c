/*
 * The generic 8259 interrupt controller, resident half.
 *
 * sys/pic.c
 *
 * pic_eoi runs at every interrupt and inton at a driver's open, so both
 * are resident.  They know nothing about which board they are on: the
 * line routing is in linepic/lineir (sys/picdata.c), filled at boot by
 * sys/pic_init.c.  The code is leaf - it reaches only in()/out()/di()/ei()
 * - and carries no static data, so it folds to .data and rides the u page
 * like the leaf runtime, which is what keeps it out of the resident text.
 */
#include <types.h>
#include <sys/pic.h>
#include <sys/pic8259.h>

extern int in(), out();
extern int di(), ei();

/*
 * Point the group select at this PIC, where there is one.  The SS1's
 * PICs are directly addressed and have no select.
 */
static
picselect(p)
    struct pic8259 *p;
{
    if (p->sel)
        out(p->sel, p->selval);
}

/*
 * End of interrupt on a line.  A vectored line is dismissed on its own
 * PIC.  A polled slave's line was already acknowledged by the poll, so it
 * is dismissed on the master's cascade alone.  A slave line dismisses the
 * master's cascade either way.
 */
pic_eoi(line)
    UINT line;
{
    struct pic8259 *p;
    UINT8 cascade;

    p = &pics[linepic[line]];
    cascade = p->cascade_ir;
    if (!p->polled) {
        picselect(p);
        out(p->icw0, 0x60 | lineir[line]);
    }
    if (cascade != 0xff) {
        p = &pics[0];           /* the master */
        picselect(p);
        out(p->icw0, 0x60 | cascade);
    }
    return (0);
}

/*
 * Unmask line a.  The "& 7" is not a no-op: the IR sits in a register and
 * c1 emits nothing for a shift by a register, so the mask forces the
 * arithmetic that moves it out.
 */
inton(a)
    UINT a;
{
    struct pic8259 *p;
    UINT8 m;

    p = &pics[linepic[a]];
    di();
    picselect(p);
    m = in(p->icw1) & ~(1 << (lineir[a] & 7));
    out(p->icw1, m);
    ei();
    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
