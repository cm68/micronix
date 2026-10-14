/*
 * The Wunderbus I/O interrupt controller, init half.
 *
 * sys/wbiopic_init.c
 *
 * pic_init brings the single 8259 up and inton/intoff/pic_irr serve the
 * boot attach loop; none runs after boot, so this whole file folds into
 * highmem.o and is reclaimed as buffer headers.  The resident half
 * (pic_eoi) is in wbiopic.c.
 *
 * A single 8259 on the Wunderbus at 0x48.  Three lines: VI0-VI2 on the
 * IR0-2.  The smallest config - no slave, no cascade.
 */
#include <types.h>
#include <sys/pic.h>

extern int in(), out();
extern int di(), ei();

#define MBASE    0x48
#define MSELECT  (MBASE + 7)    /* the group select: PENABLE picks the PIC */
#define PIC0     (MBASE + 4)    /* ICW1/OCW2/OCW3, IRR/ISR read */
#define PIC1     (MBASE + 5)    /* ICW2/ICW3/ICW4/OCW1 (the IMR) */

#define PENABLE   0x28          /* group select: the PIC */

extern char vectors[];          /* sys/intrpt.s: the table + slack */
extern int vecalign();          /* sys/picvec.c */

/*
 * Bring the 8259 up, every VI line masked.  Level triggered, 4-byte
 * vectors, single, the vector table moved to a boundary and named in the
 * ICWs.  The clock (IR7) is left unmasked; the attach loop opens the
 * VI lines.
 */
pic_init()
{
    register int base;

    base = vecalign(vectors);
    out(MSELECT, PENABLE);
    out(PIC0, 0x1f | (base & 0xe0));    /* ICW1: level, 4-byte, single, ICW4 */
    out(PIC1, (base >> 8) & 0xff);      /* ICW2: vector base high byte */
    out(PIC1, 0x0c);                    /* ICW4 */
    out(PIC1, 0xff);                    /* OCW1: mask everything */
    out(PIC1, 0x7f);                    /* leave the clock (IR7) open */

    return (0);
}

intoff(a)
{
    di();
    out(MSELECT, PENABLE);
    out(PIC1, in(PIC1) | (1 << (a & 3)));
    ei();
    return (0);
}

/*
 * Which lines are pending, as a bit mask (the master's IR0-2).
 */
pic_irr()
{
    out(MSELECT, PENABLE);
    out(PIC0, 0x0a);                    /* OCW3: read the IRR */
    return in(PIC0) & 0x07;
}
