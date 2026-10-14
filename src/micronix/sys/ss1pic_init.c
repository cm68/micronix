/*
 * The CompuPro System Support 1 interrupt controller, init half.
 *
 * sys/ss1pic_init.c
 *
 * pic_init brings both 8259s up and inton/intoff/pic_irr serve the boot
 * attach loop; none runs after boot, so this whole file folds into
 * highmem.o and is reclaimed as buffer headers.  The resident half
 * (pic_eoi) is in ss1pic.c.
 *
 * Two 8259s on the CPU board, CAS-cascaded: the master's IR7 is the
 * cascade.  Eight VI lines: VI0-VI6 on the master's IR0-6, VI7 on the
 * slave's IR0; the slave's IR1-7 are the board's own timers, FPU and
 * UART and are never touched through this interface.
 */
#include <types.h>
#include <sys/pic.h>

extern int in(), out();
extern int di(), ei();

#define MPIC0   0x50            /* master: ICW1/OCW2/OCW3, IRR/ISR read */
#define MPIC1   0x51            /* master: ICW2/ICW3/ICW4/OCW1 (the IMR) */
#define SPIC0   0x52            /* the slave's same two ports */
#define SPIC1   0x53

extern char ss1_master_vec[];   /* sys/ss1pic.s: 32 bytes of table, slack above */
extern char ss1_slave_vec[];
extern int vecalign();          /* sys/picvec.c */

/*
 * Bring both 8259s up, CAS-cascaded, every line masked.  The slave is
 * told its id is 7 (ICW3 = 7, the master's cascade line) and the master
 * is told the cascade is IR7 (ICW3 = 0x80).  Each has its own vector
 * table, moved to a boundary and named in ICW1/ICW2.
 */
pic_init()
{
    register int base;

    /* the slave first, so it is not signalling when the master arms */
    base = vecalign(ss1_slave_vec);
    out(SPIC0, 0x1d | (base & 0xe0));   /* ICW1: cascade, level, ICW4 */
    out(SPIC1, (base >> 8) & 0xff);     /* ICW2: vector base, high byte */
    out(SPIC1, 7);                      /* ICW3: slave id = master IR7 */
    out(SPIC1, 0x08);                   /* ICW4: buffered slave, 8080, EOI */
    out(SPIC1, 0xff);                   /* OCW1: mask everything */

    base = vecalign(ss1_master_vec);
    out(MPIC0, 0x1d | (base & 0xe0));   /* ICW1: cascade, level, ICW4 */
    out(MPIC1, (base >> 8) & 0xff);     /* ICW2: vector base, high byte */
    out(MPIC1, 0x80);                   /* ICW3: the cascade is IR7 */
    out(MPIC1, 0x0c);                   /* ICW4: buffered master, 8080, EOI */
    out(MPIC1, 0xff);                   /* OCW1: mask everything */
    return (0);
}

intoff(a)
{
    di();
    if (a >= 7)
        out(SPIC1, in(SPIC1) | 1);
    else
        out(MPIC1, in(MPIC1) | (1 << (a & 7)));
    ei();
    return (0);
}

/*
 * Which lines are pending, as a bit mask.  Read each 8259's IRR (OCW3
 * read-register) and fold it into line numbers: the master's IR0-6 are
 * lines 0-6, the slave's IR0 is line 7.
 */
pic_irr()
{
    register int mask;

    out(MPIC0, 0x0a);                   /* OCW3: read the master's IRR */
    mask = in(MPIC0) & 0x7f;            /* IR0-6, drop the cascade */
    out(SPIC0, 0x0a);                   /* OCW3: read the slave's IRR */
    mask |= (in(SPIC0) & 1) << 7;       /* its IR0 is line 7 */
    return (mask);
}
