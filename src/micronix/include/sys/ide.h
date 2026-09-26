/*
 * IDE controller
 *
 * include/sys/ide.h
 */

/*
 * The interrupt line the card drives.  The S-100 bus carries eight
 * vectored interrupt lines, VI0 to VI7, and a card asserts one of them;
 * which of those reach the Mult I/O board's 8259 is the board's jumper
 * area's business, and it brings in three - VI0, VI1 and VI2.  The other
 * five inputs are wired on the board itself to its own devices (three
 * serial ports, the parallel printer and the clock), so those five are
 * not bus lines at all and the three are the whole of what a card on the
 * bus can claim.  Two of the three are spoken for, and by what the
 * machine shipped with: the Decision 1 leaves the jumper area with VI0 on
 * the hard disk controller and VI1 on the floppy controller, which is the
 * jumpering Micronix is written for - mw.c claims VI0 as it comes up with
 * inton(MWINT) and cus.c claims VI1 with inton(DJINT), both of which are
 * numbers 0 and 1 because that is where the jumpers put them.
 *
 * VI2 is the one left, and it is left in the literal sense: no card on
 * the bus drives it, so nothing is behind the line.
 *
 * Past the jumpers the line is simply input 2 of the 8259, and the
 * kernel's numbering means 8259 inputs from here on.  ARMMASTER in
 * inits.s masks inputs 0, 1 and 6 and allows 2 through 7, so this one is
 * already unmasked and needs no inton() call to claim it.  sys/intrpt.s
 * routes input 2 to ideint() (sys/ide.c), and the driver's header names
 * the line for the interrupt dispatcher - the header is a different
 * object (sys/idehdr.c), so the number is here rather than beside the
 * driver.
 */
#define IDEINT  2

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
