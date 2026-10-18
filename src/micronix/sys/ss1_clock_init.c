/*
 * The CompuPro System Support 1 clock, init half.
 *
 * sys/ss1_clock_init.c
 *
 * clockinit() programs the 8253's counter 0 - the heartbeat.  It runs
 * once at boot, so this file folds into highmem.o and is reclaimed as
 * buffer headers.  The resident half (rtcinit, rtcwrite) is in
 * ss1_clock.c.
 */
#include <types.h>
#include <sys/sys.h>

/* the board's jumper block: 0x70, not CompuPro's default 0x50 - see ss1pic.c */
#define SS1_TC0      0x74    /* 8253 counter 0, the count port */
#define SS1_TCCTL    0x77    /* 8253 control word */

#define HEARTBEAT   62500   /* 2 MHz / 32 Hz, the count for the 32 Hz tick */

extern int in(), out();

/*
 * Program the 8253's counter 0, the heartbeat: 32 Hz, which the tick
 * stub (mio.s) divides by four to call clock() at 8 Hz - the same rate
 * the uPD1990 runs in wb_clock.c, so the stub is shared.  Counter 0 is a
 * 16 bit down-counter at 2 MHz, so 2000000/32 = 62500.  Mode 2 (rate
 * generator) reloads and repeats.  Its OUT is the slave 8259's IR1; the
 * unmask is the PIC's, so this only programs the timer.
 */
void
clockinit(void)
{
    out(SS1_TCCTL, 0x34);       /* counter 0, LSB then MSB, mode 2, binary */
    out(SS1_TC0, HEARTBEAT & 0xff);
    out(SS1_TC0, HEARTBEAT >> 8);
}
