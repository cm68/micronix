/*
 * The Wunderbus/MultIO interrupt controller, init half.
 *
 * sys/miopic_init.c
 *
 * pic_init brings both 8259s up and inton/intoff/pic_irr serve the boot
 * attach loop; none runs after boot, so this whole file folds into
 * highmem.o and is reclaimed as buffer headers.  The resident half
 * (pic_eoi) is in miopic.c.
 *
 * A master 8259 on the Wunderbus at 0x48 and a slave 8259 on the second
 * MultIO at 0x40.  Six lines: VI0-VI2 on the master's IR0-2, VI3-VI5 on
 * the slave's IR0-2.  The slave is polled, never CAS-vectored (the
 * Wunderbus has no CAS pins).
 */
#include <types.h>
#include <sys/pic.h>

extern int in(), out();
extern int di(), ei();

#define MBASE    0x48
#define MSELECT  (MBASE + 7)    /* the group select: PENABLE picks the PIC */
#define PIC0     (MBASE + 4)    /* master ICW1/OCW2/OCW3, IRR/ISR read */
#define PIC1     (MBASE + 5)    /* master ICW2/ICW3/ICW4/OCW1 (the IMR) */

#define SBASE    0x40
#define SSELECT  (SBASE + 7)
#define SPIC0    (SBASE + 4)
#define SPIC1    (SBASE + 5)

#define PENABLE   0x28          /* group select: the PIC */
#define SPICONLY  0x08          /* group select: the slave's PIC only */

#define CASCADE_IR 6            /* the master IR the slave's INT is wired to */

extern char vectors[];          /* sys/intrpt.s: the master table + slack */
extern int vecalign();          /* sys/picvec.c */

/*
 * Bring both 8259s up, every VI line masked.  The master is level
 * triggered, 4-byte vectors, single (no ICW3 - the slave is polled, not
 * CAS), and its vector table is moved to a boundary and named in the
 * ICWs.  The slave takes ICW3 for its id (the master's cascade line) and,
 * polled, a zero vector base.  The clock (IR7) and cascade (IR6) are left
 * unmasked; the attach loop opens the VI lines.
 */
pic_init()
{
    register int base;

    /* the master */
    base = vecalign(vectors);
    out(MSELECT, PENABLE);
    out(PIC0, 0x1f | (base & 0xe0));    /* ICW1: level, 4-byte, single, ICW4 */
    out(PIC1, (base >> 8) & 0xff);      /* ICW2: vector base high byte */
    out(PIC1, 0x0c);                    /* ICW4 */
    out(PIC1, 0xff);                    /* OCW1: mask everything */

    /* the slave */
    out(SSELECT, SPICONLY);
    out(SPIC0, 0x1d);                   /* ICW1: level, cascade, ICW4 */
    out(SPIC1, 0x00);                   /* ICW2: no vector, the slave is polled */
    out(SPIC1, CASCADE_IR);             /* ICW3: slave id = the master's IR6 */
    out(SPIC1, 0x00);                   /* ICW4 */
    out(SPIC1, 0xff);                   /* OCW1: mask everything */

    /*
     * Leave the board's own sources open: the three master ACEs on
     * IR3-IR5, the slave's cascade on IR6 and the clock on IR7.  Only
     * the three VI lines on IR0-IR2 are masked.  The attach loop opens
     * the six VI lines - IR0-IR2 here and the slave's IR0-IR2 - but the
     * ACEs are not VI lines and nothing in the pic.h interface can name
     * them: inton(3) is VI3, the slave's IR0.  Masking IR3-IR5 here
     * left the console's transmit interrupt, the only thing that drains
     * its output queue, masked forever.
     */
    out(MSELECT, PENABLE);
    out(PIC1, 0x07);                    /* OCW1: mask IR0-2, open IR3-7 */

    return (0);
}

intoff(a)
{
    di();
    if (a >= 3) {
        out(SSELECT, SPICONLY);
        out(SPIC1, in(SPIC1) | (1 << (a - 3)));
    } else {
        out(MSELECT, PENABLE);
        out(PIC1, in(PIC1) | (1 << (a & 3)));
    }
    ei();
    return (0);
}

/*
 * Which lines are pending, as a bit mask.  Read each 8259's IRR (OCW3
 * read-register); the master's IR0-2 are lines 0-2, the slave's IR0-2
 * lines 3-5.
 */
pic_irr()
{
    register int mask;

    out(MSELECT, PENABLE);
    out(PIC0, 0x0a);                    /* OCW3: read the master's IRR */
    mask = in(PIC0) & 0x07;
    out(SSELECT, SPICONLY);
    out(SPIC0, 0x0a);                   /* OCW3: read the slave's IRR */
    mask |= (in(SPIC0) & 0x07) << 3;
    return (mask);
}
