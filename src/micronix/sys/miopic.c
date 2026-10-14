/*
 * The Wunderbus/MultIO interrupt controller, resident half.
 *
 * sys/miopic.c
 *
 * pic_eoi() runs at every interrupt and inton() at a driver's open, so
 * both are resident; the boot-only bring-up - pic_init, intoff, pic_irr -
 * lives in miopic_init.c, folded into highmem and reclaimed.
 */
#include <types.h>
#include <sys/pic.h>

extern int in(), out();
extern int di(), ei();

#define MBASE    0x48
#define MSELECT  (MBASE + 7)    /* the group select: PENABLE picks the PIC */
#define PIC0     (MBASE + 4)    /* master ICW1/OCW2/OCW3, IRR/ISR read */
#define PIC1     (MBASE + 5)    /* master ICW2/ICW3/ICW4/OCW1 (the IMR) */

#define SSELECT  (0x58 + 7)
#define SPIC1    (0x58 + 5)     /* the slave's IMR/OCW1 register */

#define PENABLE   0x28          /* group select: the PIC */
#define SPICONLY  0x08          /* group select: the slave's PIC only */

#define CASCADE_IR 6            /* the master IR the slave's INT is wired to */

/*
 * End of interrupt on a line.  A master line (0-2) is dismissed by the
 * master alone.  A slave line (3-5) is dismissed by the master's cascade
 * EOI - the slave's poll was its own acknowledge.
 */
pic_eoi(line)
{
    if (line >= 3)
        out(PIC0, 0x60 | CASCADE_IR);   /* specific EOI, master IR6 */
    else
        out(PIC0, 0x60 | line);         /* specific EOI, master IRn */
    return (0);
}

/*
 * Unmask line a.  Lines 0-2 are the master's IR0-2, 3-5 the slave's
 * IR0-2 (reached by selecting the slave).  The "& 3" is not a no-op: a
 * sits in BC and c1 emits nothing for a shift by a register, so the mask
 * forces the arithmetic that moves it out.
 */
inton(a)
{
    di();
    if (a >= 3) {
        out(SSELECT, SPICONLY);
        out(SPIC1, in(SPIC1) & ~(1 << (a - 3)));
    } else {
        out(MSELECT, PENABLE);
        out(PIC1, in(PIC1) & ~(1 << (a & 3)));
    }
    ei();
    return (0);
}
