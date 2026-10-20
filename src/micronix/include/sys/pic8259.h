/*
 * The generic 8259, resident data.
 *
 * include/sys/pic8259.h
 *
 * The interrupt controller is one resident 8259 driver and a table of the
 * PICs the boot found.  Each PIC is described by its two register ports,
 * its group select (0 where none) and how it attaches to the master: a
 * master has cascade_ir 0xff, a slave the master IR its INT is wired to.
 * A line number maps through linepic/lineir to a PIC and an IR on it.
 *
 * The table lives in resident data (sys/picdata.c) because it is shared
 * by every process and mutated once at boot; the resident code that walks
 * it (sys/pic.c) is leaf code in the u page.
 */

struct pic8259 {
    UINT8 icw0;         /* ICW1/OCW2/OCW3, IRR/ISR read */
    UINT8 icw1;         /* ICW2/ICW3/ICW4/OCW1 (the IMR) */
    UINT8 sel;          /* group-select port, 0 for a directly-addressed PIC */
    UINT8 selval;       /* what is written to sel to pick this PIC */
    UINT8 cascade_ir;   /* the master IR this PIC cascades through; 0xff = master */
    UINT8 polled;       /* 1 = polled slave, its poll was the acknowledge */
};

#define NPIC        3   /* master + one slave + one polled slave */
#define MAXNOVLINE  8   /* the SS1's eight VI lines is the widest */

extern struct pic8259 pics[NPIC];
extern UINT8 linepic[MAXNOVLINE];   /* line -> pics[] index */
extern UINT8 lineir[MAXNOVLINE];    /* line -> IR number on that PIC */
extern UINT8 npics, nlines;         /* how many of each the boot found */

/* the clock and the real-time clock, chosen by the same probe.  The two
 * boards provide the same three entry points (ss1_clock.c, upd1990.c);
 * pic_init points these at whichever answered. */
extern int (*clockinit)();          /* the heartbeat */
extern int (*rtcinit)();            /* set the clock from s_time */
extern int (*rtcwrite)();           /* read the clock as s_time */

/* the console, chosen by the same probe.  The SS1's 2651 (ss1_uart.c)
 * and the MultIO's ACE (mio.s) provide the same entry points. */
extern int (*console_putc)();       /* print a character */
extern int (*console_start)();      /* enable output */
extern int (*console_stop)();       /* disable output */
extern int (*console_set)();        /* set baud rate (no-op on the 2651) */
extern int (*console_minit)();      /* the uart bring-up */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
