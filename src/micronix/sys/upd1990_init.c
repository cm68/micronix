/*
 * The Wunderbus/MultIO clock, init half.
 *
 * sys/upd1990_init.c
 *
 * upd1990_clockinit() programs the uPD1990's periodic output - the heartbeat.
 * It runs once at boot, so this file folds into highmem.o and is
 * reclaimed as buffer headers.  The resident half (rtcinit, rtcwrite) is
 * in upd1990.c.
 */
#include <types.h>
#include <sys/sys.h>

#define CLOCKPORT   0x4a            /* MBASE + 2 */
#define SELPORT     0x4f            /* MBASE + 7, the group select */

#define CLK_SETCMD  0x20            /* CSTROBE */
#define CC_SET      0x08            /* shift register -> clock */
#define CC_32HZ     0x1c            /* the periodic output, 32 Hz */

extern int in(), out();

/*
 * Program the uPD1990's periodic output - the heartbeat.  32 Hz, which
 * the tick stub (mio.s) divides by four to call clock() at 8 Hz.  The
 * chip is in group 0 of the MultIO, so select it first; this used to be
 * the clock block in _minit (inits.s).
 */
void
upd1990_clockinit(void)
{
    out(SELPORT, 0);                    /* select group 0 (control) */

    out(CLOCKPORT, CC_SET);             /* time-set mode */
    out(CLOCKPORT, CC_SET | CLK_SETCMD);
    out(CLOCKPORT, CC_SET);

    out(CLOCKPORT, CC_32HZ);            /* the rate */
    out(CLOCKPORT, CC_32HZ | CLK_SETCMD);
    out(CLOCKPORT, CC_32HZ);

    in(CLOCKPORT);                      /* clear the clock latch */
}
