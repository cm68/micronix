#include <sys/ints.h>

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
 * The card names no interrupt line: the resident attach loop
 * (sys/probe.c) tickles it and discovers the line it raised.
 */

/*
 * The card's ports.  Eight consecutive Z80 I/O addresses - a number space
 * of its own, separate from memory, reached with in() and out() rather
 * than by a load or a store (inout.s) - and the register number is the low
 * three bits of the address, A0 to A2 straight into the chip.
 *
 * The range is free on this machine: 0x30-0x34 is the MYIDE board,
 * 0x40-0x47 and 0x48-0x4f the Mult I/O console, 0x50-0x53 the obsolete
 * HDCA, 0x54-0x55 the HD-DMA, 0xd0-0xd1 the simulator's own ports, and
 * 0xef the DJ-DMA.  A SCSI host adapter on this bus has always been
 * strapped at 0x40, which the console's second board now claims, so this
 * machine puts the card at 0x20.
 */
#define SCSIBASE    0x20        /* register 0; 0x20..0x27 are registers 0..7 */

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
