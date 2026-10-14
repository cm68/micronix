/*
 * The interrupt controller interface.
 *
 * include/sys/pic.h
 *
 * One object, chosen at link time, provides the five entry points below
 * and nothing else.  The drivers and the resident half call them by
 * name; the link resolves them to the chosen object (ss1pic, miopic or
 * wbpic), so nothing outside that object knows which PIC stack is
 * present or where its registers are.  sys/INTR.md is the design.
 *
 * The entry points speak in line numbers, 0..NOVLINE-1, where a line is
 * one S-100 vectored interrupt line (VI0..).  Which line lands on which
 * 8259 and which of its IR inputs is the object's business alone.
 *
 * NOVLINE is the number of lines this build carries.  It is a
 * compile-time constant because the tables sized with it (ovlint[] in
 * sys/probe.c) are fixed arrays.  The build passes it in with
 * -DNOVLINE=... together with the object choice, so the two cannot
 * disagree:
 *
 *      wbpic   -DNOVLINE=3     one 8259, three lines (VI0-VI2)
 *      miopic  -DNOVLINE=6     master + polled slave, six lines (VI0-VI5)
 *      ss1pic  -DNOVLINE=8     two CAS 8259s, eight lines (VI0-VI7)
 */
#ifndef NOVLINE
#define NOVLINE 3
#endif

extern int pic_init();          /* bring the PIC(s) up, every line masked */
extern int inton();             /* inton(line): unmask a line */
extern int intoff();            /* intoff(line): mask a line */
extern int pic_irr();           /* which lines are pending, as a bit mask */
extern int pic_eoi();           /* pic_eoi(line): end of interrupt on a line */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
