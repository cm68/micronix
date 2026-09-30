/*
 * NCR 5380 SCSI host adapter
 *
 * include/sys/ncr.h
 *
 * The resident side's view of the card, and the whole of it: the
 * interrupt line, the port the registers answer at, and the one register
 * the presence probe writes.  The driver itself is a module
 * (docs/DRIVERS.md) and is named for the adapter - sys/ncr.c -
 * while the device nodes stay scsi*, because the next host adapter to be
 * supported here will be a different chip behind the same bus.
 *
 * These are here rather than beside the driver for the reason sys/ide.h
 * gives: sys/ncrhdr.c, the header the loader reads, and sys/ncrinit.c,
 * the probe, are both resident, and resident code cannot name anything in
 * a module - neither a function nor a data object.  A number it can
 * carry.
 */

/*
 * The interrupt line the card drives, and it is not the card's own.
 *
 * ide.c reads the S-100 jumpers out: the Mult I/O board brings in three
 * of the bus's eight vectored interrupt lines - VI0, VI1 and VI2 - and
 * the other five inputs are wired on the board itself to its own devices
 * (three serial ports, the printer and the clock), so those five are not
 * bus lines at all.  All three are spoken for: VI0 is mw.c's HD-DMA, VI1
 * is cus.c's floppy, VI2 is ide.c.
 *
 * VI2 and not one of the others because VI2 is the one the bus leaves
 * free in the literal sense - the MYIDE board is the only thing that has
 * ever driven it, and a machine with a SCSI disk has no particular reason
 * to have that board in it as well.  Sharing a line is not a special
 * case the way it used to be: the 5380 brings its own interrupt down when
 * its registers are read, the IDE card brings its own down the same way,
 * and ovlntr (sys/ovl.c) calls both handlers in one pass over the line.
 * Neither driver knows about the other; the fan-out is resident, and the
 * routine that used to chain them by hand (scsii2int) is gone.
 *
 * Past the jumpers the line is simply input 2 of the 8259, and ARMMASTER
 * in inits.s already unmasks it, so no inton() call claims it.
 */
#define NCRINT  2

/*
 * The card's ports.  Eight consecutive Z80 I/O addresses - a number space
 * of its own, separate from memory, reached with in() and out() rather
 * than by a load or a store (inout.s) - and the register number is the low
 * three bits of the address, A0 to A2 straight into the chip.
 *
 * The range is free on this machine: 0x30-0x34 is the MYIDE board,
 * 0x48-0x4f and 0x58-0x5f the Mult I/O console, 0x50-0x53 the obsolete
 * HDCA, 0x54-0x55 the HD-DMA, 0xd0-0xd1 the simulator's own ports, and
 * 0xef the DJ-DMA.  0x40 is what a SCSI host adapter on this bus has
 * always been strapped to, and nothing else here wants it.
 */
#define SCSIBASE    0x40        /* register 0; 0x40..0x47 are registers 0..7 */

/*
 * The initiator command register, and the presence probe's whole test.
 *
 * A port nothing answers reads as 0xff on this bus, and a bare read of
 * one tells the two cases apart only if a card that IS there reads
 * something else - which the current bus status register cannot promise:
 * its bits are the bus's own, active low, so a card sitting on an idle
 * bus reads 0xff exactly as an empty socket does.  The command register
 * is the cpu's own latch rather than the bus's, on a 5380 as much as in
 * the simulator, and that is what makes it worth reading back.
 *
 * So the probe (sys/ncrinit.c) writes 0 to it and reads it: the six
 * writable bits are the latch, they were just cleared, and a card that
 * is there answers with all six clear.  The two bits the mask below
 * leaves out are the chip's own and are never set by writing - 0x40,
 * arbitration in progress, and 0x20, loss of arbitration - and neither
 * is set on a card at rest, so masking rather than comparing accepts
 * them and still fails on 0xff.
 */
#define ICR     1               /* initiator command: SEL, BSY, ACK, RST... */
#define ICR_WR  0x9f            /* its writable bits: RST, ACK, BSY, SEL,
                                   ATN and DATA - the six ncr.c defines */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
