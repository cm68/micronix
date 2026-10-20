/*
 * The generic 8259 interrupt controller, init half.
 *
 * sys/pic_init.c
 *
 * pic_init brings the 8259s up and fills the resident line table;
 * intoff and pic_irr serve the boot attach loop.  None runs after boot,
 * so this whole file folds into highmem.o and is reclaimed as buffer
 * headers.  The resident half (pic_eoi, inton) is in sys/pic.c.
 *
 * The board is discovered, not assumed: the master is brought up, the
 * slave is programmed and then probed to see whether it answered, and the
 * line table is filled from what is there.  (The SS1 probe joins ahead of
 * this.)
 */
#include <types.h>
#include <sys/pic.h>
#include <sys/pic8259.h>

extern int in(), out();
extern int di(), ei();

extern char vectors[];          /* sys/intrpt.s: the WB/MultIO master table */
extern char ss1_master_vec[], ss1_slave_vec[]; /* sys/ss1vec.s */
extern int vecalign();          /* sys/picvec.c */

/* the two clock boards' entry points (ss1_clock*.c, upd1990*.c) */
extern int ss1_clockinit(), ss1_rtcinit(), ss1_rtcwrite();
extern int upd1990_clockinit(), upd1990_rtcinit(), upd1990_rtcwrite();

/* the two console boards' entry points (ss1_uart.c, mio.s/inits.s) */
extern int ss1_putc(), ss1_start(), ss1_stop(), ss1_set(), ss1_uart_init();
extern int ace_putc(), ace_start(), ace_stop(), ace_set(), ace_minit();

/* the CompuPro SS1's two CAS-cascaded 8259s */
#define SS1MPIC0 0x70
#define SS1MPIC1 0x71
#define SS1SPIC0 0x72
#define SS1SPIC1 0x73

/* the Wunderbus master and the polled MultIO slave */
#define MBASE    0x48
#define MSELECT  (MBASE + 7)
#define PIC0     (MBASE + 4)
#define PIC1     (MBASE + 5)

#define SBASE    0x40
#define SSELECT  (SBASE + 7)
#define SPIC0    (SBASE + 4)
#define SPIC1    (SBASE + 5)

#define PENABLE   0x28
#define SPICONLY  0x08
#define CASCADE_IR 2           /* the slave yanks on VI2, the master's IR2 */

/*
 * Point the group select at this PIC, where there is one.
 */
static
picselect(p)
    struct pic8259 *p;
{
    if (p->sel)
        out(p->sel, p->selval);
}

/*
 * Did the 8259 at p answer?  Its ICWs are already programmed, so OCW1 is
 * the IMR: write two values and read them back.  A missing PIC reads open
 * bus - the same value whatever is written - so both have to round-trip.
 */
static
picprobe(p)
    struct pic8259 *p;
{
    UINT8 a, b;

    picselect(p);
    out(p->icw1, 0x00);
    a = in(p->icw1);
    out(p->icw1, 0xff);
    b = in(p->icw1);
    out(p->icw1, 0xff);         /* leave it masked */
    return (a == 0x00 && b == 0xff);
}

/*
 * Bring the board up and fill the resident table.  The master is level
 * triggered, 4-byte vectors, single (no ICW3 - the slave is polled), and
 * its vector table is moved to a boundary and named in the ICWs.  The
 * slave, if it answered, takes ICW3 for its id and, polled, a zero vector
 * base.  The clock (IR7) and, with a slave, the cascade (IR2) are left
 * unmasked; the attach loop opens the VI lines.
 */
pic_init()
{
    register int base;
    UINT8 i;

    /*
     * The SS1 first: two CAS-cascaded 8259s on the CPU board.  Program
     * them and probe the master; a card that is not there takes the
     * writes harmlessly and never reads them back.
     */
    pics[0].icw0 = SS1MPIC0;
    pics[0].icw1 = SS1MPIC1;
    pics[0].sel = 0;
    pics[0].selval = 0;
    pics[0].cascade_ir = 0xff;
    pics[0].polled = 0;
    pics[1].icw0 = SS1SPIC0;
    pics[1].icw1 = SS1SPIC1;
    pics[1].sel = 0;
    pics[1].selval = 0;
    pics[1].cascade_ir = 7;
    pics[1].polled = 0;

    /* the slave first, so it is not signalling when the master arms */
    base = vecalign(ss1_slave_vec, 0x220);
    out(SS1SPIC0, 0x1d | (base & 0xe0));   /* ICW1: cascade, level, ICW4 */
    out(SS1SPIC1, (base >> 8) & 0xff);     /* ICW2: vector base high byte */
    out(SS1SPIC1, 7);                      /* ICW3: slave id = master IR7 */
    out(SS1SPIC1, 0x08);                   /* ICW4: buffered slave */
    out(SS1SPIC1, 0xff);                   /* OCW1: mask everything */
    base = vecalign(ss1_master_vec, 0x200);
    out(SS1MPIC0, 0x1d | (base & 0xe0));   /* ICW1: cascade, level, ICW4 */
    out(SS1MPIC1, (base >> 8) & 0xff);     /* ICW2: vector base high byte */
    out(SS1MPIC1, 0x80);                   /* ICW3: the cascade is IR7 */
    out(SS1MPIC1, 0x0c);                   /* ICW4: buffered master */
    out(SS1MPIC1, 0xff);                   /* OCW1: mask everything */

    if (picprobe(&pics[0])) {
        /* the SS1 answered: eight lines */
        npics = 2;
        nlines = 8;
        for (i = 0; i < 7; i++) {       /* VI0-6, the master's IR0-6 */
            linepic[i] = 0;
            lineir[i] = i;
        }
        linepic[7] = 1;                 /* VI7, the slave's IR0 */
        lineir[7] = 0;
        clockinit = ss1_clockinit;
        rtcinit = ss1_rtcinit;
        rtcwrite = ss1_rtcwrite;
        console_putc = ss1_putc;
        console_start = ss1_start;
        console_stop = ss1_stop;
        console_set = ss1_set;
        console_minit = ss1_uart_init;
        out(SS1MPIC1, 0x7f);            /* mask IR0-6, open IR7 (the cascade) */
        out(SS1SPIC1, 0x7d);            /* open the timer (IR1) and RxRDY
                                         * (IR7); VI7 is the attach loop's.
                                         * TxRDY (IR6) stays closed until the
                                         * console tty exists (ss1_uart.c) -
                                         * the monitor leaves the 2651
                                         * transmitter on, so it is already
                                         * asking */
        return (0);
    }

    /* no SS1: the Wunderbus master, then the (optional) slave */
    pics[0].icw0 = PIC0;
    pics[0].icw1 = PIC1;
    pics[0].sel = MSELECT;
    pics[0].selval = PENABLE;
    pics[0].cascade_ir = 0xff;
    pics[0].polled = 0;
    pics[1].icw0 = SPIC0;
    pics[1].icw1 = SPIC1;
    pics[1].sel = SSELECT;
    pics[1].selval = SPICONLY;
    pics[1].cascade_ir = CASCADE_IR;
    pics[1].polled = 1;

    /* the master */
    base = vecalign(vectors, 0x200);
    out(MSELECT, PENABLE);
    out(PIC0, 0x1f | (base & 0xe0));    /* ICW1: level, 4-byte, single, ICW4 */
    out(PIC1, (base >> 8) & 0xff);      /* ICW2: vector base high byte */
    out(PIC1, 0x0c);                    /* ICW4 */
    out(PIC1, 0xff);                    /* OCW1: mask everything */

    /* the slave, programmed then probed - a card that is not there takes
     * the writes harmlessly and never reads them back */
    out(SSELECT, SPICONLY);
    out(SPIC0, 0x1d);                   /* ICW1: level, cascade, ICW4 */
    out(SPIC1, 0x00);                   /* ICW2: no vector, polled */
    out(SPIC1, CASCADE_IR);             /* ICW3: slave id = the master's IR2 */
    out(SPIC1, 0x00);                   /* ICW4 */
    out(SPIC1, 0xff);                   /* OCW1: mask everything */

    if (picprobe(&pics[1])) {
        /* the slave answered: five lines, VI2 is the cascade */
        npics = 2;
        nlines = 5;
        for (i = 0; i < 2; i++) {       /* VI0-1, the master's IR0-1 */
            linepic[i] = 0;
            lineir[i] = i;
        }
        for (i = 0; i < 3; i++) {       /* VI3-5, the slave's IR0-2 */
            linepic[2 + i] = 1;
            lineir[2 + i] = i;
        }
        out(MSELECT, PENABLE);
        out(PIC1, 0x03);                /* mask IR0-1, open IR2 (cascade) + IR3-7 */
    } else {
        /* the single Wunderbus: three lines */
        npics = 1;
        nlines = 3;
        for (i = 0; i < 3; i++) {       /* VI0-2, the master's IR0-2 */
            linepic[i] = 0;
            lineir[i] = i;
        }
        out(MSELECT, PENABLE);
        out(PIC1, 0x07);                /* mask IR0-2, open IR3-7 */
    }

    clockinit = upd1990_clockinit;
    rtcinit = upd1990_rtcinit;
    rtcwrite = upd1990_rtcwrite;
    console_putc = ace_putc;
    console_start = ace_start;
    console_stop = ace_stop;
    console_set = ace_set;
    console_minit = ace_minit;
    return (0);
}

/*
 * Mask line a.
 */
intoff(a)
    UINT a;
{
    struct pic8259 *p;
    UINT8 m;

    p = &pics[linepic[a]];
    di();
    picselect(p);
    m = in(p->icw1) | (1 << (lineir[a] & 7));
    out(p->icw1, m);
    ei();
    return (0);
}

/*
 * Which lines are pending, as a bit mask.  Read each PIC's IRR and fold
 * each line's bit in.  The output bit walks with a variable rather than
 * (1 << l), which c1 would emit nothing for.
 */
pic_irr()
{
    register UINT8 i, l;
    register int mask, irr, bit;

    mask = 0;
    for (i = 0; i < npics; i++) {
        picselect(&pics[i]);
        out(pics[i].icw0, 0x0a);        /* OCW3: read the IRR */
        irr = in(pics[i].icw0);
        bit = 1;
        for (l = 0; l < nlines; l++) {
            if (linepic[l] == i) {
                if (irr & 1)
                    mask |= bit;
                irr >>= 1;
            }
            bit <<= 1;
        }
    }
    return (mask);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
