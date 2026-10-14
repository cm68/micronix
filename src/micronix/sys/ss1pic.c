/*
 * The CompuPro System Support 1 interrupt controller, resident half.
 *
 * sys/ss1pic.c
 *
 * pic_eoi() runs at every interrupt and inton() at a driver's open, so
 * both are resident; the boot-only bring-up - pic_init, intoff, pic_irr
 * - lives in ss1pic_init.c, folded into highmem and reclaimed.
 */
#include <types.h>
#include <sys/pic.h>

extern int in(), out();
extern int di(), ei();

#define MPIC0   0x50            /* master: ICW1/OCW2/OCW3, IRR/ISR read */
#define MPIC1   0x51            /* master: ICW2/ICW3/ICW4/OCW1 (the IMR) */
#define SPIC0   0x52            /* the slave's same two ports */
#define SPIC1   0x53

/*
 * End of interrupt on a line.  A master line (0-6) is dismissed by the
 * master alone.  Line 7 is the slave: it is dismissed first, then the
 * master's cascade (IR7), which stays asserted until the slave is told.
 */
pic_eoi(line)
{
    if (line >= 7) {
        out(SPIC0, 0x60);               /* specific EOI, slave IR0 */
        out(MPIC0, 0x67);               /* specific EOI, master IR7 */
    } else {
        out(MPIC0, 0x60 | line);        /* specific EOI, master IRn */
    }
    return (0);
}

/*
 * Unmask line a.  Lines 0-6 are the master's IR0-6, line 7 the slave's
 * IR0.  The "& 7" is not a no-op: a sits in BC and c1 has no rule for
 * shifting by a register, so the mask forces the arithmetic that moves it
 * out.
 */
inton(a)
{
    di();
    if (a >= 7)
        out(SPIC1, in(SPIC1) & ~1);         /* the slave's IR0 */
    else
        out(MPIC1, in(MPIC1) & ~(1 << (a & 7)));
    ei();
    return (0);
}
