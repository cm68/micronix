/*
 * The Wunderbus I/O interrupt controller, resident half.
 *
 * sys/wbiopic.c
 *
 * pic_eoi() runs at every interrupt and inton() at a driver's open, so
 * both are resident; the boot-only bring-up - pic_init, intoff, pic_irr
 * - lives in wbiopic_init.c, folded into highmem and reclaimed.
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

/*
 * End of interrupt on a line: specific EOI on the master.
 */
pic_eoi(line)
{
    out(PIC0, 0x60 | line);
    return (0);
}

/*
 * Unmask line a, 0-2 on the IR0-2.  The "& 3" is not a no-op: a sits in
 * BC and c1 emits nothing for a shift by a register, so the mask forces
 * the arithmetic that moves it out.
 */
inton(a)
{
    di();
    out(MSELECT, PENABLE);
    out(PIC1, in(PIC1) & ~(1 << (a & 3)));
    ei();
    return (0);
}
