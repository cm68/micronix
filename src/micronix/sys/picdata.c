/*
 * The generic 8259 table, resident data.
 *
 * sys/picdata.c
 *
 * The PIC table and the line routing.  It is resident because it is
 * shared by every process and because the leaf code that walks it
 * (sys/pic.c, in the u page) reaches it by absolute address.  It is
 * filled once, at boot, by sys/pic_init.c.
 */
#include <types.h>
#include <sys/pic8259.h>

struct pic8259 pics[NPIC];
UINT8 linepic[MAXNOVLINE];
UINT8 lineir[MAXNOVLINE];
UINT8 npics, nlines;

/* the clock and RTC, chosen by the same probe (sys/pic_init.c) */
int (*clockinit)();
int (*rtcinit)();
int (*rtcwrite)();

/* the console, chosen by the same probe */
int (*console_putc)();
int (*console_start)();
int (*console_stop)();
int (*console_set)();
int (*console_minit)();

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
