/*
 * ncr5380.c - a SCSI host adapter built on the NCR 5380, from the far end.
 *
 * The other end of sys/ncr.c.  That file is the authority for everything
 * below - which port a register answers at, which bit means what, and the
 * order the wires are moved in - and this one is the target's half of it:
 * the disk that answers, the handshake that carries the bytes, and the
 * interrupt that ends the command.  Where the two seem to disagree the
 * driver is right, because it is what runs.
 *
 * Terms, since the rest of this file leans on them:
 *
 *   SCSI        a bus, not a drive: up to eight devices on it, each with a
 *               number of its own called its id, one of them talking (the
 *               initiator) and one answering (the target).  Here the card
 *               is always the initiator and a disk behind it is always the
 *               target.
 *   NCR 5380    the bus interface chip: eight registers the cpu reads and
 *               writes and a set of bus transceivers.  The register is the
 *               low three bits of the port - A0-A2 straight into the chip -
 *               so 0x40..0x47 are registers 0..7 (include/sys/ncr.h).  It
 *               does not know what a disk is; it drives the wires.
 *   CDB         command descriptor block: the ten bytes that ARE the
 *               command - an opcode, an LBA and a block count.  READ(10)
 *               and WRITE(10) are the only two sys/ncr.c builds.
 *   LBA         the whole disk as one run of 512-byte blocks numbered from
 *               0.  SCSI is addressed that way natively, and it is what the
 *               driver hands over.
 *   phase       what the bus is being used for at this instant: COMMAND,
 *               DATA IN, DATA OUT, STATUS, MESSAGE IN, and BUS FREE between
 *               commands.  Three wires name it - MSG, C/D and I/O - and the
 *               target owns them.
 *   REQ, ACK    the handshake, one byte at a time, and the reason this card
 *               has no state a clock advances: the target raises REQ when it
 *               has a byte or wants one, the initiator moves the byte and
 *               raises ACK, the target drops REQ, the initiator drops ACK.
 *               One byte is one round trip, and the byte engine below is
 *               stepped by the initiator's reads of the bus status register.
 *   SEL, BSY    the two wires the two ends claim the bus with.  The card
 *               raises both to call a target, the target answers by raising
 *               BSY of its own, and BSY going away is how a command ends.
 *   VI2, PIC    the S-100 bus's vectored interrupt lines and the 8259 on the
 *               Mult I/O board which ranks them.  This card shares VI2 with
 *               the IDE card and claims it with card id 1; see below.
 *
 * WHAT IS NOT MODELLED, AND WHY THAT IS NOT A SHORTCUT
 *
 * Writes to registers 5, 6 and 7 start a DMA transfer on a real 5380, and
 * they do nothing here: this machine has no DMA controller wired to the
 * card, so sys/ncr.c moves every byte itself through the data register with
 * the handshake above, and a transfer nothing would answer is better left
 * unarmed than half modelled.  Nothing checks parity either - the driver
 * never sets the mode register's parity bit, and its comment says why - so
 * the parity error latch stays clear.  The registers a target uses (the
 * target command register, the select enable register) are latched and not
 * acted on, because an initiator is what the driver is.
 *
 * THE INTERRUPT
 *
 * A 5380 in initiator mode with no DMA controller has one thing to say
 * asynchronously, and sys/ncr.c builds on exactly it: the bus went free,
 * which is MONITOR BSY (mode register bit 2) and is the end of a command.
 * The chip holds IRQ until the reset/parity/interrupt register is read, and
 * that read is the acknowledgement - the 8259 here is level triggered and
 * intrupt (mio.s) writes the end-of-interrupt when the handler returns, so
 * a card that held the line up past that would be serviced twice for one
 * command.  The busy and status register is read first: reading RPI clears
 * the parity, IRQ and busy-error latches together, and the status register
 * is the only place the reason survives.
 *
 * The line is open collector and shared, which is what the card id is for
 * (s100.c:set_vi): a card that takes its own request down says which card
 * it is, and the line stays up for as long as any other card on it is still
 * asking.  ide.c is id 0 on VI2; this is id 1.
 *
 * THE DISK BEHIND A TARGET
 *
 * One image per target id - unit scsi-0 through scsi-7, handed over by
 * hwsim.c's drivearg or found in the -d directory - for the same reason the
 * device nodes are scsi*: the target is the disk's number on the bus and
 * nothing else.  The geometry is the image's own label, read the way ide.c
 * reads it, and the LBA a command names is decomposed into cylinder, head
 * and sector the same way too.  So a volume laid out for the HD-DMA reads
 * back block for block here, which is the whole compatibility argument, and
 * it is applied in the same place it is for every other disk on this
 * machine.
 *
 * There is no boot path: mon447 and mon500 have no SCSI code in them, so
 * this card is not in hwsim.c's bootdevs[] and -B does not name it.
 */

#include "sim.h"
#include "hwsim.h"
#include "util.h"
#include <strings.h>
#include <stdio.h>

#ifndef NODEBUG
int trace_scsi;
extern int trace_bio;
#endif

/*
 * The card's ports.  Eight consecutive Z80 I/O addresses with the register
 * number in the low three bits, so one pair of handlers serves all eight
 * and the register is p & 7.  The range is free on this machine: 0x30-0x34
 * is the MYIDE board, 0x48-0x4f and 0x58-0x5f the Mult I/O console,
 * 0x50-0x53 the obsolete hdca, 0x54-0x55 the hd-dma, 0xd0-0xd1 the
 * simulator's own, 0xef the dj-dma, and 0x40 is what a SCSI host adapter
 * on this bus has always been strapped to.
 */
#define SCSIBASE    0x40
#define NREG        8

/*
 * The register numbers, in the card's own order.  Three of the eight mean
 * one thing read and another written, which is why the names below are the
 * read side's where they differ; the write side is in the handler.
 */
#define R_CSD       0           /* read: current data  write: output data */
#define R_ICR       1           /* initiator command */
#define R_MR        2           /* mode */
#define R_TCR       3           /* target command: written, never acted on */
#define R_CSBS      4           /* read: current bus status  write: select enable */
#define R_BASR      5           /* read: bus and status  write: start DMA send */
#define R_IDR       6           /* read: input data  write: start DMA receive */
#define R_RPI       7           /* read: reset parity/interrupt (the ack) */

/*
 * Initiator command register.  The six bits the cpu owns; the two above
 * them are the chip's (arbitration in progress, lost arbitration) and are
 * always clear here because nothing on this bus arbitrates.  The mask is
 * the one the guest's presence probe reads back through - ICR_WR,
 * include/sys/ncr.h - and it is written out again here rather than shared
 * because a resident probe and a host simulator have no header between
 * them.
 */
#define ICR_RST     0x80        /* assert RST: the bus's reset line */
#define ICR_ACK     0x10        /* assert ACK, our half of the handshake */
#define ICR_BSY     0x08        /* assert BSY, our claim on the bus */
#define ICR_SEL     0x04        /* assert SEL */
#define ICR_ATN     0x02        /* assert ATN: a message is coming */
#define ICR_DATA    0x01        /* our output data register drives the bus */
#define ICR_WR      0x9f        /* which of them a write can set */

/*
 * Mode register.  The driver writes it twice in its life - 0 to disarm and
 * MR_MONBSY to arm - and the one bit that reaches this file is the one that
 * makes the bus going free an interrupt.  Everything else stays clear:
 * this is not a target, it does not DMA, it does not arbitrate (see
 * scsiselect in sys/ncr.c), and it never asks the chip to check parity.
 */
#define MR_MONBSY   0x04

/*
 * Current SCSI bus status register, read.  A set bit is an asserted signal
 * on the bus.  The target drives MSG, C/D, I/O, REQ and BSY; the initiator
 * drives SEL and RST.
 */
#define CSBS_RST    0x80
#define CSBS_BSY    0x40
#define CSBS_REQ    0x20
#define CSBS_MSG    0x10
#define CSBS_CD     0x08
#define CSBS_IO     0x04
#define CSBS_SEL    0x02

/*
 * The three phase wires together name the phase, and the values below are
 * the encoding's (sys/ncr.c's names, and unused by the driver - it never
 * reads the phase wires, it polls REQ and lets the phase be whatever the
 * byte it wanted arrives in).  Keeping them straight costs nothing and
 * makes the trace read like a bus trace.
 */
#define PH_COMMAND  CSBS_CD
#define PH_DATAOUT  0
#define PH_DATAIN   CSBS_IO
#define PH_STATUS   (CSBS_CD | CSBS_IO)
#define PH_MSGIN    (CSBS_CD | CSBS_IO | CSBS_MSG)

/*
 * Bus and status register, read.  EOP is the end of a DMA transfer and
 * there is no DMA; PHASE is target mode's comparison of the bus phase
 * against the target command register, which this driver never writes.
 */
#define BASR_EOP    0x80
#define BASR_DRQ    0x40
#define BASR_PERR   0x20
#define BASR_IRQ    0x10
#define BASR_PHASE  0x08
#define BASR_BUSY   0x04
#define BASR_ATN    0x02
#define BASR_ACK    0x01

/*
 * The status byte the target sends in the STATUS phase, and the message
 * byte of the MESSAGE IN phase.  CHECK CONDITION is the whole of what this
 * target has to say about a command it would not do: sys/ncr.c acts on it
 * by failing the request, and asking why would mean REQUEST SENSE, which it
 * does not send.
 */
#define ST_GOOD     0x00
#define ST_CHECK    0x02
#define MSG_CMDCOMP 0x00

/*
 * The two commands the driver builds.  Ten bytes each - opcode, LUN and
 * LBA, a reserved byte, the block count and a control byte - and the block
 * count is always one, because a buffer here is one 512-byte block.
 */
#define CREAD10     0x28
#define CWRITE10    0x2a
#define CREADCAP    0x25        /* READ CAPACITY (10): last LBA + block size */
#define CDBLEN      10

/*
 * The interrupt line the card drives and the id it claims it by.  VI2 is
 * shared with the IDE card, which is id 0 on it; the ids are per line and
 * not global, and hddma/hdca are the same arrangement on VI0.
 */
#define SCSI_INTERRUPT  2
#define SCSI_CARDID     1

/*
 * How many targets the bus can hold, and the transfer unit.  Eight is the
 * bus's own limit and not a choice: the select mask is one bit per id in a
 * single byte, and the target number is the low three bits of the device
 * number (sys/ncr.c's ncropen).
 */
#define NTARGET     8
#define SECLEN      512

/*
 * The disk behind a target, opened the first time that target is called and
 * kept: a target that answers and then has its geometry refused is still a
 * target, and reopening the image on every selection would relock it.
 */
static void *handle[NTARGET];   /* the image behind a target, or null */
static int geomok[NTARGET];     /* ... and whether its label gave a geometry */
static int cyls[NTARGET];       /* that geometry, for the capacity */
static int heads[NTARGET];      /* ... and the LBA's decomposition */
static int spt[NTARGET];

/*
 * The card, all of it.  There is one card and one command on the bus at a
 * time - sys/ncr.c holds a busy flag and lets one requester past it - so
 * nothing here is reentrant and nothing needs to be.  The one thing that
 * happens asynchronously is the interrupt line, and the cpu takes that
 * between instructions and never inside a port access.
 */
static byte odr;                /* output data register: what we drive */
static byte icr;                /* initiator command register */
static byte mr;                 /* mode register */
static byte tcr;                /* target command register, never acted on */
static byte selwr;              /* select enable, written to 4, never read */
static byte cdb[CDBLEN];        /* the command being handed over */
static byte buf[SECLEN];        /* the block being moved */
static byte stbyte;             /* the status byte the target will send */
static int state;               /* which phase the target has the bus in */
static int target;              /* the id being called, or -1 */
static int bsy;                 /* the target's claim on the bus */
static int req;                 /* the target has a byte, or wants one */
static int datain;              /* the data phase moves bytes toward the cpu */
static int n;                   /* which byte of the phase, counted from 0 */
static int dlen;                /* how many bytes the data phase is */
static int refused;             /* the command cannot be done; the status says so */
static unsigned long lba;       /* the block the command in flight named */
static int req_low;             /* REQ just dropped, and not yet read as low */
static int owed;                /* ... and the byte after it is due */
static int irq;                 /* the chip's IRQ latch, cleared by RPI */
static int pbusy;               /* the BUSY ERROR latch, cleared by RPI */

/*
 * The phases the target moves the bus through.  BUS FREE is the one the
 * card is in when nobody is talking, and SELECTION is the gap between the
 * initiator putting the id bits on the bus and letting go of it.
 */
#define BFREE       0
#define BSEL        1
#define BCMD        2
#define BDATA       3
#define BSTAT       4
#define BMSG        5

/*
 * One target's image, and whether there is a usable one.  Returns 0 if a
 * formatted target is there to answer a selection, -1 if there is no disk
 * behind the id or the image has no geometry - either way nothing comes
 * back with BSY, which is what tells the driver nobody is home rather than
 * leaving it to spin out its bounded poll on a command that was never
 * going to be answered.  This mirrors ide.c: an empty bay is invisible on
 * the bus, not a target that refuses the command.
 */
static int
ncr_unit(int id)
{
    char name[20];
    struct drive *dh;

    if (handle[id])
        return (0);

    sprintf(name, "scsi-%d", id);
    dh = drive_open(name);
    if (!dh) {
        printf("scsi: no target %s, selection unanswered\n", name);
        return (-1);
    }

    drive_sectorsize(dh, SECLEN);
    geomok[id] = drive_geometry(dh, &cyls[id], &heads[id], &spt[id]) &&
        heads[id] && spt[id];
    if (!geomok[id]) {
        printf("scsi: target %s has no geometry, command refused\n", name);
        return (-1);
    }
    handle[id] = dh;                /* only cache a formatted drive */
    trace(trace_scsi, "scsi: target %d is %d/%d/%d\n", id, cyls[id],
        heads[id], spt[id]);
    return (0);
}

/*
 * One block, in or out, at an LBA.
 *
 * The LBA is turned back into the cylinder, head and sector the guest
 * computed it from - the driver's scsilba() is the other direction - and
 * those are what the drive module wants.  Its own arithmetic then
 * re-flattens them, so the two cancel and the file offset is the LBA times
 * the sector size.  Doing it in that order is the compatibility argument:
 * handing the module the LBA as a byte offset would be shorter and would
 * read a different disk.
 */
static int
ncr_rw(int id, unsigned long lba, char *p, int wr)
{
    int spc = heads[id] * spt[id];
    int cyl = lba / spc;
    int rem = lba % spc;
    int head = rem / spt[id];
    int sec = rem % spt[id];

    trace(trace_scsi, "scsi: %s lba %lu = c %d h %d s %d\n",
        wr ? "write" : "read", lba, cyl, head, sec);

    if (wr)
        return (drive_write(handle[id], cyl, head, sec, p));
    return (drive_read(handle[id], cyl, head, sec, p));
}

/*
 * Put the chip's interrupt pin on the bus line, or take it off.  The chip
 * holds IRQ until RPI is read and no enable gates it after the fact, so the
 * request is the latch and the latch is what this is driven from - which is
 * what makes it idempotent, and why every path that moves the latch ends
 * here rather than at set_vi (ide.c's ide_update_irq is the same shape).
 */
static void
ncr_update_irq(void)
{
    set_vi(SCSI_INTERRUPT, SCSI_CARDID, irq ? 1 : 0);
}

/*
 * The bus reset line, which is the initiator's to hold down and every
 * device's to obey.  Asserting it is not one more register write: it is
 * every target letting go of the bus at once, so a command in flight is
 * abandoned here, and the latches go with it - a card that kept a pending
 * interrupt through its own bus reset would ask again for a command that no
 * longer exists.
 *
 * The chip's own command register is not cleared: on a 5380 the RST bit
 * drives the bus and the chip's /RESET pin clears the registers, which is
 * why myreset() in sys/ncr.c writes ICR back to zero and then the mode
 * register, in that order.
 */
static void
ncr_reset_bus(void)
{
    state = BFREE;
    target = -1;
    bsy = 0;
    req = 0;
    req_low = 0;
    owed = 0;
    n = 0;
    irq = 0;
    pbusy = 0;
    ncr_update_irq();
    trace(trace_scsi, "scsi: bus reset\n");
}

/*
 * The byte the register file puts on the data bus.  On the SCSI bus the
 * direction is I/O's business: I/O asserted means the target drives, and
 * that is exactly the test here.  It matters more than it looks, because
 * sys/ncr.c's byte loop ends with ICR_DATA and leaves its own data drivers
 * on across the step from the command phase into a data-in phase, so the
 * read of the first data byte happens while the initiator is still driving
 * the bus.  A chip that read its own output latch back there would hand the
 * driver the last byte of the CDB 512 times.  The wire is the target's and
 * the register is the wire.
 */
static byte
ncr_data(void)
{
    if (!bsy)
        return (odr);

    switch (state) {
    case BDATA:
        if (datain && n < dlen)
            return (buf[n]);
        return (odr);
    case BSTAT:
        return (stbyte);
    case BMSG:
        return (MSG_CMDCOMP);
    }
    return (odr);               /* the command phase: the initiator's own byte */
}

/*
 * The three phase wires, as the target is holding them.  They are for the
 * trace and for the phase-match bit of the status register; sys/ncr.c polls
 * REQ and never looks at them, so nothing the driver does depends on the
 * values here.
 */
static byte
ncr_phase(void)
{
    switch (state) {
    case BCMD:
        return (PH_COMMAND);
    case BDATA:
        return (datain ? PH_DATAIN : PH_DATAOUT);
    case BSTAT:
        return (PH_STATUS);
    case BMSG:
        return (PH_MSGIN);
    }
    return (0);
}

/*
 * The end of a command: the target lets go of BSY and the bus is free.
 *
 * This is the one moment the card has anything to say asynchronously, and
 * MONITOR BSY is what makes it an interrupt: with the bit clear the bus
 * going free is just the bus going free, which is what the driver wants
 * across the reset in its own probe.  With it set the chip latches the
 * reason - BSY went away with a command in flight, which is the BUSY ERROR
 * bit - and holds IRQ until RPI is read.
 */
static void
ncr_free(void)
{
    state = BFREE;
    target = -1;
    bsy = 0;
    req = 0;
    req_low = 0;
    owed = 0;

    if (mr & MR_MONBSY) {
        pbusy = 1;
        irq = 1;
    }
    ncr_update_irq();
    trace(trace_scsi, "scsi: bus free\n");
}

/*
 * The command phase is over: the ten bytes are in the card's command block
 * and this is what the target makes of them.
 *
 * The two commands move one block each, and a buffer here is one block, so
 * a count of anything else is a command this target will not do.  A read's
 * block is fetched now, before the first byte of the data phase is offered,
 * because the driver is entitled to read the data register the moment REQ
 * comes up; a write's is stored when the last byte has arrived (ncr_advance).
 *
 * A refused command is still a command that runs: the data phase happens
 * either way, with the block the buffer was cleared to, and the status byte
 * is what reports the failure.  That is not politeness - the driver decides
 * the direction of the transfer from the buffer it is moving and not from
 * the opcode, so a target that skipped the data phase would have the
 * driver reading a status byte as block data and then spinning out 512
 * bounded polls for bytes that are never coming.  ide.c refuses by setting
 * an error bit in a status register and letting the command finish; this is
 * the same thing, in the register SCSI has for it.
 */
static void
ncr_command(void)
{
    int count;
    unsigned long last;

    refused = 0;
    stbyte = ST_GOOD;
    datain = 1;
    n = 0;
    bzero(buf, SECLEN);

    switch (cdb[0]) {
    case CREAD10:
        datain = 1;
        break;
    case CWRITE10:
        datain = 0;
        break;
    case CREADCAP:
        datain = 1;
        break;
    default:
        /*
         * Nothing else is ever built (scsibuild, sys/ncr.c), and an
         * unknown opcode has no direction to move the bus in, so it is
         * answered as a refused read - the one shape that cannot leave
         * the driver polling into an empty phase whichever way it meant
         * to go.
         */
        l("scsi: unknown opcode 0x%x\n", cdb[0]);
        refused = 1;
        break;
    }

    if (cdb[1] & 0xe0) {
        l("scsi: lun %d is not this target's\n", cdb[1] >> 5);
        refused = 1;
    }

    /*
     * READ CAPACITY's cdb[6..7] are reserved, not a block count, and its
     * data phase is the eight bytes the target reports; every other
     * command moves one block.
     */
    if (cdb[0] == CREADCAP) {
        dlen = 8;
    } else {
        count = (cdb[6] << 8) | cdb[7];
        if (count != 1) {
            l("scsi: %d blocks asked for, this target moves one\n", count);
            refused = 1;
        }
        dlen = SECLEN;
    }

    /*
     * The LBA, in the order scsibuild() lays it down: five bits in the
     * command's second byte under the LUN, then bits 15-8, 7-0, and the
     * top byte last.
     */
    lba = ((unsigned long)(cdb[1] & 0x1f) << 16) |
        ((unsigned long)cdb[2] << 8) | (unsigned long)cdb[3] |
        ((unsigned long)cdb[4] << 24);

    if (!geomok[target]) {
        l("scsi: target %d has no geometry, command refused\n", target);
        refused = 1;
    }

    if (!refused && datain) {
        if (cdb[0] == CREADCAP) {
            /*
             * Last LBA and the block size, the two words READ CAPACITY
             * reports, both big-endian.  The capacity is the whole of the
             * geometry the label gave, and the block is SECLEN.
             */
            last = (unsigned long)cyls[target] * heads[target] *
                spt[target] - 1;
            buf[0] = last >> 24;
            buf[1] = last >> 16;
            buf[2] = last >> 8;
            buf[3] = last;
            buf[4] = 0;
            buf[5] = 0;
            buf[6] = (SECLEN >> 8) & 0xff;
            buf[7] = SECLEN & 0xff;
        } else if (ncr_rw(target, lba, buf, 0) != SECLEN) {
            l("scsi: read of lba %lu failed\n", lba);
            refused = 1;
        }
#ifndef NODEBUG
        if (traceflags & trace_bio)
            hexdump(buf, dlen);
#endif
    }

    if (refused)
        stbyte = ST_CHECK;

    state = BDATA;
    req = 1;
}

/*
 * The byte engine: the target is ready for the byte after the one the
 * initiator has just taken, so either offer the next one or move the bus on
 * to the phase that follows.
 *
 * It is called from the read of the bus status register and from nowhere
 * else, because that read is the only thing in this card that takes time -
 * there is no clock here, and the handshake is what a clock would be
 * doing.  See ncr_csbs for why it runs one read behind the byte.
 */
static void
ncr_advance(void)
{
    req = 0;

    switch (state) {
    case BCMD:
        if (n < CDBLEN) {
            req = 1;
            return;
        }
        ncr_command();
        return;
    case BDATA:
        if (n < dlen) {
            req = 1;
            return;
        }
        if (!datain && !refused) {
            if (ncr_rw(target, lba, buf, 1) != SECLEN) {
                l("scsi: write of lba %lu failed\n", lba);
                stbyte = ST_CHECK;
            }
#ifndef NODEBUG
            if (traceflags & trace_bio)
                hexdump(buf, SECLEN);
#endif
        }
        state = BSTAT;
        n = 0;
        req = 1;
        return;
    case BSTAT:
        state = BMSG;
        n = 0;
        req = 1;
        return;
    case BMSG:
        ncr_free();
        return;
    }
}

/*
 * The initiator has raised ACK: the byte on the bus is across, and the
 * target's answer to that is to take REQ down.
 *
 * REQ going down is not the same as the initiator having seen it go down,
 * and sys/ncr.c's byte loop shows why the difference matters - it moves the
 * byte, drops ACK, and then waits for REQ to be clear before it will return,
 * so a target that raised REQ again the moment ACK fell would never be seen
 * to have dropped it.  So the two are kept apart: the drop is recorded
 * here, and the next byte is offered by the next read of the bus status
 * register after the one that reads REQ low.  One observation of the low is
 * the least that can stand for "the initiator has been told", and on a bus
 * with no clock it is the only thing that can.
 */
static void
ncr_ack(void)
{
    switch (state) {
    case BCMD:
        if (n < CDBLEN)
            cdb[n] = odr;
        n++;
        break;
    case BDATA:
        if (!datain && n < dlen)
            buf[n] = odr;
        n++;
        break;
    case BSTAT:
    case BMSG:
        n++;                    /* the byte was read before ACK went up */
        break;
    default:
        return;
    }
    req = 0;
    req_low = 1;
}

/*
 * Call a target and wait for it to answer.
 *
 * No arbitration: the card takes the bus rather than competing for it, the
 * way sys/ncr.c's scsiselect() does and for the reason it gives.  The id
 * bits are on the data bus, SEL says a selection is happening and BSY
 * claims the bus, and the target answers by raising BSY of its own.
 *
 * It is the TARGET's BSY the driver's poll has to see, and the one thing in
 * this file that is not a literal model of the wires: the initiator is
 * holding BSY down itself at that moment, so a chip that reported its own
 * claim back would answer its own poll and the driver's "nobody home" test
 * (sys/ncr.c:602-610) could never fire - it would go on to write a CDB to
 * nothing and spin out its per-byte bound 512 times instead of failing the
 * request.  BSY is reported here as the other end's claim, which is what
 * the driver is asking about.
 */
static void
ncr_select(void)
{
    int id;

    /*
     * The mask is (1 << HOSTID) | (1 << target) with the host at 7, so
     * the target's bit is the one below it that is set.  No bit at all
     * is nobody called, and the bus is left alone.
     */
    for (id = 0; id < NTARGET - 1; id++)
        if (odr & (1 << id))
            break;
    if (id == NTARGET - 1)
        return;

    if (ncr_unit(id) < 0)
        return;

    target = id;
    state = BSEL;
    bsy = 1;                    /* the answer the driver is polling for */
    trace(trace_scsi, "scsi: select %d\n", id);
}

/*
 * The bus status register, read - and the card's clock, because it is the
 * only register the handshake reads.
 *
 * The order is the whole of it: the byte that is owed is offered first, so
 * that the read which sees REQ low is the read before it, and then this
 * read arms the one after.  A read that comes when nothing is owed is just
 * a read, which is what the driver's bus-free poll and its end-of-command
 * status test are.
 */
static byte
ncr_csbs(void)
{
    byte st;

    if (owed) {
        owed = 0;
        ncr_advance();
    }

    st = 0;
    if (icr & ICR_RST)
        st |= CSBS_RST;
    if (bsy)
        st |= CSBS_BSY | ncr_phase();
    if (req)
        st |= CSBS_REQ;
    if (icr & ICR_SEL)
        st |= CSBS_SEL;

    if (req_low) {
        req_low = 0;
        owed = 1;
    }
    return (st);
}

/*
 * The bus and status register, read.  The driver's handler tests PERR and
 * BUSY and calls the command over on either, so those two latches have to
 * be the ones its own reading put there: BUSY is BSY going away with a
 * command in flight, and PERR is nothing at all, because no parity is
 * generated or checked here.
 */
static byte
ncr_basr(void)
{
    byte st;

    st = 0;
    if (req)
        st |= BASR_DRQ;         /* the DMA request line is REQ in this mode */
    if (pbusy)
        st |= BASR_BUSY;
    if (irq)
        st |= BASR_IRQ;
    if ((ncr_phase() << 1) == (tcr & 0x38))
        st |= BASR_PHASE;
    if (icr & ICR_ATN)
        st |= BASR_ATN;
    if (icr & ICR_ACK)
        st |= BASR_ACK;
    return (st);
}

/*
 * One register, read: the register number is the low three bits of the
 * port, and the two that share a byte with the data bus answer the same
 * thing (sys/ncr.c's IDR is documented as "the same byte as CSD").
 */
static byte
rd_scsi(portaddr p)
{
    switch (p & 7) {
    case R_CSD:
    case R_IDR:
        return (ncr_data());
    case R_ICR:
        return (icr & ICR_WR);
    case R_MR:
        return (mr);
    case R_TCR:
        return (tcr);
    case R_CSBS:
        return (ncr_csbs());
    case R_BASR:
        return (ncr_basr());
    case R_RPI:
        /*
         * The acknowledgement, and it is not optional: the 5380 holds
         * IRQ down until this register is read and the 8259 is level
         * triggered, so the line would fire again the moment intrupt
         * wrote its end-of-interrupt.  Reading it clears the parity,
         * IRQ and busy-error latches together, which is why the status
         * register has to be read first - it is where the reason
         * survives.  What it reads as is nothing the driver looks at.
         */
        irq = 0;
        pbusy = 0;
        ncr_update_irq();
        return (0);
    }
    return (0xff);
}

/*
 * One register, written.  Register 0 is the output data latch, 1 the
 * command register and its handshake, 2 the mode register; 3 and 4 are the
 * registers a target uses, latched and not acted on; and 5, 6 and 7 arm a
 * DMA transfer, which is where this card's model of the chip stops (see the
 * header).
 */
static void
wr_scsi(portaddr p, byte v)
{
    byte old;

    switch (p & 7) {
    case R_CSD:
        odr = v;
        return;
    case R_ICR:
        old = icr;
        icr = v;

        if ((v & ICR_RST) && !(old & ICR_RST))
            ncr_reset_bus();

        /*
         * A selection: the id mask on the data bus and SEL to say what
         * it is.  The target answers by raising BSY - see ncr_select for
         * whose BSY that has to be.
         */
        if ((v & (ICR_SEL | ICR_DATA)) == (ICR_SEL | ICR_DATA) &&
            !(old & ICR_SEL) && state == BFREE)
            ncr_select();

        /*
         * The initiator has let go of SEL and BSY: the selection is
         * over and the target takes the bus into the command phase,
         * asking for the first byte of the CDB.
         */
        if (state == BSEL && !(v & ICR_SEL) && !(v & ICR_BSY)) {
            state = BCMD;
            n = 0;
            req = 1;
        }

        if ((v & ICR_ACK) && !(old & ICR_ACK))
            ncr_ack();
        return;
    case R_MR:
        mr = v;
        return;
    case R_TCR:
        tcr = v;
        return;
    case R_CSBS:
        /*
         * The select enable register, which a 5380 being an initiator
         * uses to say how it will answer a reselection.  Nothing here
         * reselects and the driver never writes it; it is latched so
         * that a write is not a write to nothing.
         */
        selwr = v;
        return;
    default:
        /*
         * 5, 6 and 7 start a DMA transfer.  There is no DMA controller
         * on this machine, the driver moves every byte through the data
         * register, and a transfer nothing would answer is better left
         * unarmed than half started.
         */
        trace(trace_scsi, "scsi: DMA reg %d written 0x%x, ignored\n",
            p & 7, v);
        return;
    }
}

static int
ncr_init()
{
    int i;

    for (i = 0; i < NREG; i++) {
        register_output(SCSIBASE + i, &wr_scsi);
        register_input(SCSIBASE + i, &rd_scsi);
    }

    /*
     * The card comes up with the bus free and the mode register clear, so
     * MONITOR BSY is off until the driver arms it in its own reset
     * (myintr, sys/ncr.c).  A card that came up asking for attention
     * would be asking about a command nobody has issued.
     */
    mr = 0;
    odr = 0;
    icr = 0;
    tcr = 0;
    selwr = 0;
    ncr_reset_bus();
    return (0);
}

static int
ncr_setup()
{
#ifndef NODEBUG
    trace_scsi = register_trace("scsi");
#endif
    return (0);
}

struct driver ncr_driver = {
    "scsi",
    0,
    &ncr_setup,
    &ncr_init,
    0
};

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_ncr_driver()
{
    register_driver(&ncr_driver);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
