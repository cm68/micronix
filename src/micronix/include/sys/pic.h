/*
 * The interrupt controller interface.
 *
 * include/sys/pic.h
 *
 * One resident 8259 driver provides the five entry points below and
 * nothing else.  pic_init() discovers the board at boot - two CAS 8259s
 * on the SS1, a master plus a polled slave on the MultIO, or a single
 * Wunderbus PIC - and fills the resident line table (sys/pic8259.h);
 * the entry points then route through it.  sys/INTR.md is the design.
 *
 * The entry points speak in line numbers, 0..nlines-1, where a line is
 * one S-100 vectored interrupt line (VI0..).  Which line lands on which
 * 8259 and which of its IR inputs is discovered, not compiled in.
 */
extern UINT8 pic_init();          /* bring the PIC(s) up, every line masked */
extern UINT8 inton();             /* inton(line): unmask a line */
extern UINT8 intoff();            /* intoff(line): mask a line (init only) */
extern UINT8 pic_mask();          /* pic_mask(line): mask a line, resident */
extern INT16 pic_irr();           /* which lines are pending, as a bit mask */
extern UINT8 pic_eoi();           /* pic_eoi(line): end of interrupt on a line */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
