/*
 * ide.c - the S100Computers MYIDE board.
 *
 * A board that puts an 8255 PPI between the S-100 bus and the pins of an
 * ATA drive, and does nothing else.  Terms, since the rest of this file
 * leans on them:
 *
 *   8255 PPI    programmable peripheral interface: three 8-bit ports,
 *               called A, B and C, whose direction is set by writing a
 *               mode word to a fourth address.  The board wires ports A
 *               and B to the drive's sixteen data lines and port C to
 *               its address and control pins.
 *   ATA, IDE    attached (formerly AT) attachment, integrated drive
 *               electronics: the interface and command set the drive
 *               speaks.  The drive has its own controller inside it, so
 *               this board is a shim and knows nothing about cylinders -
 *               it is told an address and passes it on.
 *   task file   the drive's register set: data, error (features when
 *               written), sector count, three holding an LBA, drive/head,
 *               and status (command when written).  Eight registers,
 *               numbered 0 to 7.  "File" because each is addressed like
 *               a word of memory.
 *   LBA         logical block address: the whole disk as one run of
 *               512-byte sectors numbered from 0.
 *   /CS0, /CS1  chip selects, and /RD, /WR, /RST the read, write and
 *               reset strobes.  A leading / marks a signal asserted by
 *               pulling it low, which is why the board inverts most of
 *               port C on its way to the drive, and why a set bit in the
 *               port C latch means the line is asserted.  /CS0 reaches
 *               the task file; /CS1 reaches the two registers ATA keeps
 *               apart from it, the device control register and the
 *               alternate status register.
 *   INTRQ       the drive's interrupt request, and nIEN its enable: bit 1
 *               of the device control register, active low.  INTRQ goes up
 *               when the drive has found a sector and wants to hand it
 *               over, and down again when the status register is read -
 *               which is why that read serves as the acknowledgement.
 *   unit        one drive.  The board has a latch picking which of its
 *               two the strobes reach; it is not a task file register.
 *
 * The task file is not mapped into the bus at all.  Port C carries the
 * register number and the strobes, port A carries the byte, and port B is
 * the high half of a 16-bit transfer.  Which is why the 16-bit word is the
 * unit of a sector transfer here and not a speed trick: the guest takes
 * ports A and B inside one /RD, and two reads of port A under two strobes
 * would advance the drive a word each and hand back the low half twice.
 *
 * Two guests talk to this card and both are authority on the protocol:
 * sys/ide.c in the kernel, and the boot driver in mon500.s.  Changing how
 * a port behaves means changing what the three of them agree on.
 *
 * The card has one interrupt line, VI2, which nothing else on this machine
 * drives.  A guest cannot both poll the status register and be told by
 * interrupt about the same command, because the read that watches is the
 * read that acknowledges - so sys/ide.c sleeps on the interrupt and takes
 * the status in its handler, while mon500.s and stand/boot/ideio.c poll
 * and never connect the line at all.  Both work, which is why the drive's
 * interrupt enable is written by whoever wants it rather than assumed:
 * ATA comes out of reset with it on, and only one of the three is
 * entitled to rely on that.
 *
 * The card addresses its drives by LBA, because that is what a guest
 * sends it.  The file behind the unit is a flat image of the disk whose
 * sector (c, h, s) sits at DATAOFF + (s + h*spt + c*spt*heads) * secsize
 * (lib/hdcdmadisk.c), and the label's geometry makes that a flat function
 * of the LBA the guest computed: ((cyl * heads + head) * spt + sec).  So
 * the LBA is decomposed with the drive's own geometry and handed to
 * drive_read/drive_write, and a volume written for the hd-dma reads back
 * block for block through here.
 */

#include "sim.h"
#include "hwsim.h"
#include "util.h"
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <stdio.h>

#ifndef NODEBUG
int trace_ide;
extern int trace_bio;
#endif

/*
 * The board's ports.  The range is free on this machine: 0x48-0x4f and
 * 0x58-0x5f are the Mult I/O console, 0x50-0x53 the obsolete hdca,
 * 0x54-0x55 the hd-dma, 0xd0-0xd1 the simulator's own, and 0xef the
 * dj-dma.
 */
#define IDE_PA      0x30        /* 8255 port A: drive data, low byte */
#define IDE_PB      0x31        /* 8255 port B: drive data, high byte */
#define IDE_PC      0x32        /* 8255 port C: register address, strobes */
#define IDE_PCTL    0x33        /* 8255 mode word */
#define IDE_PDRV    0x34        /* drive select latch */

/*
 * 8255 mode words.  Port C is an output either way; what changes is
 * whether ports A and B are inputs (a register read, or a sector coming
 * off the drive) or outputs (a register write, or a sector going to it).
 */
#define PPI_RD      0x92        /* A and B in, C out */
#define PPI_WR      0x80        /* A, B and C all out */

/*
 * Port C bits.  A0-A2 reach the drive's address pins directly; the rest
 * go through inverting buffers, so a set bit here asserts the line.
 */
#define C_A0        0x01
#define C_A1        0x02
#define C_A2        0x04
#define C_CS0       0x08        /* chip select, the task file's half */
#define C_CS1       0x10        /* the other half: device control */
#define C_WR        0x20
#define C_RD        0x40
#define C_RST       0x80

/*
 * Hooking the register number rather than masking it here keeps the one
 * line that says A0-A2 are the drive's address pins in a single place.
 */
#define C_REG(v)    ((v) & 0x07)

/*
 * Task file register numbers.  These are ATA's names and not ports: the
 * board turns one into whatever its bus needs.  Status and command are
 * one address on the drive's pins and the direction picks which is meant,
 * so nothing here has to know which of the two it is being asked for.
 */
#define R_DATA      0           /* data register */
#define R_ERRST     1           /* error (read) / features (write) */
#define R_SCOUNT    2           /* sector count */
#define R_LBA0      3           /* sector number / LBA 0-7 */
#define R_LBA1      4           /* cylinder low  / LBA 8-15 */
#define R_LBA2      5           /* cylinder high / LBA 16-23 */
#define R_DRVHD     6           /* drive/head    / LBA 24-27 */
#define R_STAT      7           /* status (read) / command (write) */
#define NREG        8

/*
 * The two registers /CS1 reaches instead, at the same two register
 * numbers: read 7 under /CS1 is the alternate status register, the same
 * byte as status but without the acknowledgement, and write 6 under /CS1
 * is the device control register.  Only one bit of the latter is wired to
 * anything here - the drive's interrupt enable - because the other is a
 * software reset, and this board resets the drive with /RST instead.
 */
#define R_CTL       6           /* device control, written through /CS1 */
#define NIEN        0x02        /* ...and its interrupt enable, low = on */

/*
 * The interrupt line the card drives, and the card id it claims it by.
 * The bus's vectored lines are open collector and can be shared, so the
 * id is how a card that releases the line says which one it is (s100.c);
 * nothing else on this machine drives VI2, so this is the first id on it.
 * sys/intrpt.s routes the line to sys/ide.c's handler.
 */
#define IDE_INTERRUPT   2
#define IDE_CARDID      0

/*
 * Status register bits.
 */
#define SBSY        0x80        /* busy */
#define SDRDY       0x40        /* drive ready */
#define SDRQ        0x08        /* data request */
#define SERR        0x01        /* error */

/*
 * Commands.  Both transfer one sector and retry internally, which is the
 * whole of what a guest needs: the drive is addressed by LBA and finds
 * the sector itself, so there is no geometry to send it.
 */
#define CREAD       0x20        /* read sector(s), with retry */
#define CWRITE      0x30        /* write sector(s), with retry */
#define CIDENTIFY   0xec        /* identify device: geometry and capacity */

#define SECLEN      512         /* a sector, and the transfer unit */
#define NWORD       (SECLEN / 2)

#define DRIVES      2           /* one board, and the latch picks its two */

static void *handle[DRIVES];    /* the file behind each unit, or null */
static int cyls[DRIVES];        /* geometry, as the drive's label has it */
static int heads[DRIVES];
static int spt[DRIVES];

/*
 * The board, all of it.  There is one card and it holds one transfer at
 * a time: a guest either polls the drive to completion before it starts
 * another, or sleeps on the interrupt and moves the sector when it wakes
 * with the cpu's interrupts off.  Either way the ports are touched by one
 * guest at a time, so nothing here is reentrant and nothing needs to be -
 * raising the interrupt line is all this card does asynchronously, and the
 * cpu takes that between instructions rather than inside a port access.
 */
static byte mode;               /* last mode word written to the PPI */
static byte porta, portb;       /* the port A and B output latches */
static byte drvsel;             /* the drive select latch */
static byte tf[NREG];           /* the task file, as the guest left it */
static byte status;             /* status register, composed here */
static byte buf[SECLEN];        /* the sector being moved */
static int wpos;                /* which 16-bit word of it, 0..NWORD */
static int drq;                 /* a transfer is asking for data */
static int writing;             /* ... and it is going to the drive */
static byte rdlo, rdhi;         /* what ports A and B answer right now */
static int nien;                /* the drive's interrupt enable, 1 = cut */
static int irq;                 /* the drive wants the guest's attention */

/*
 * Point the board's strobes at one of its two drives and make sure the
 * file behind it is open and its geometry known.  Returns 0, or -1 if
 * there is no image there: a unit with nothing behind it has to fail the
 * command rather than be dereferenced, and drive_open answers null when
 * the image is locked against us or cannot be opened.
 */
static int
ide_unit(int unit)
{
    char name[20];
    int c, h, s;
    struct drive *dh;

    if (handle[unit])
        return (0);

    sprintf(name, "ide-%d", unit);
    dh = drive_open(name);
    if (!dh) {
        printf("ide: no drive %s, command refused\n", name);
        return (-1);
    }
    drive_sectorsize(dh, SECLEN);
    if (!drive_geometry(dh, &c, &h, &s) || !h || !s) {
        printf("ide: drive %s has no geometry, command refused\n", name);
        return (-1);
    }
    handle[unit] = dh;              /* only cache a formatted drive */
    cyls[unit] = c;
    heads[unit] = h;
    spt[unit] = s;
    trace(trace_ide, "ide: unit %d is %d/%d/%d\n", unit, c, h, s);
    return (0);
}

/*
 * One sector, in or out, at an LBA.
 *
 * The LBA is turned back into the cylinder, head and sector the guest
 * computed it from - idelba() in sys/ide.c is the other direction - and
 * those are what the drive module wants.  Its own arithmetic then
 * re-flattens them, so the two cancel and the file offset is the LBA
 * times the sector size.  Doing it in that order is the compatibility
 * argument: handing the module the LBA as a byte offset would be shorter
 * and would read a different disk.
 */
static int
ide_rw(int unit, unsigned long lba, char *p, int wr)
{
    int spc = heads[unit] * spt[unit];
    int cyl = lba / spc;
    int rem = lba % spc;
    int head = rem / spt[unit];
    int sec = rem % spt[unit];

    trace(trace_ide, "ide: %s lba %lu = c %d h %d s %d\n",
        wr ? "write" : "read", lba, cyl, head, sec);

    if (wr)
        return (drive_write(handle[unit], cyl, head, sec, p));
    return (drive_read(handle[unit], cyl, head, sec, p));
}

/*
 * Put the drive's interrupt pin on the bus line, or take it off.  INTRQ
 * reaches VI2 only while the drive's interrupt enable is clear, so this is
 * the one place that decides, and every path that moves either the request
 * or the enable goes through it - a card that changed its mind without
 * saying so would leave the line up with nothing behind it, and the 8259
 * is level triggered, so it would ask again for as long as the line stayed
 * up.
 */
static void
ide_update_irq()
{
    set_vi(IDE_INTERRUPT, IDE_CARDID, (irq && !nien) ? 1 : 0);
}

/*
 * What the drive comes up as: ready, and not asking for anything.  A real
 * drive raises busy and drops it again after a reset, and the guests poll
 * for exactly that (sys/ide.c's idereset), but the busy is over by the
 * time anything here could be asked about it, so only the settled state
 * is modelled.
 *
 * The reset reaches the device control register as well as the task file -
 * /RST is the drive's own reset pin - so the interrupt enable goes back to
 * its power-on value of on, and a guest that wants to hear from the drive
 * has to leave it that way.
 */
static void
ide_reset()
{
    int i;

    drq = 0;
    writing = 0;
    wpos = 0;
    irq = 0;
    nien = 0;
    for (i = 0; i < NREG; i++)
        tf[i] = 0;
    status = SDRDY;
    if (ide_unit(0) < 0)
        status = 0;             /* empty unit-0 bay: not ready */
    ide_update_irq();
    trace(trace_ide, "ide: reset\n");
}

/*
 * The LBA the guest put in the task file: four bits in the drive/head
 * register, the rest in the three below it.  A drive larger than 28 bits
 * of sector would not fit, which is a limit of the addressing mode and
 * not of this board.
 */
static unsigned long
ide_lba()
{
    unsigned long lba;

    lba = ((unsigned long)(tf[R_DRVHD] & 0x0f) << 24);
    lba |= ((unsigned long)tf[R_LBA2] << 16);
    lba |= ((unsigned long)tf[R_LBA1] << 8);
    lba |= (unsigned long)tf[R_LBA0];
    return (lba);
}

/*
 * A command has landed in the command register.  Both commands this board
 * sees move one sector, so both end up in the same place: the drive asks
 * for data, and the transfer that follows is what actually does the work.
 *
 * The sector is read here rather than when the first word is asked for,
 * because the guest polls for DRQ before it starts transferring and the
 * drive has to be ready to answer when it does.
 *
 * Every road out of here leaves the drive with something to report - a
 * sector ready, or a refusal - so the request is raised once, at the top,
 * and the status set on the way out says which it is.  The line itself is
 * left to the caller, because raising it is what the caller does after any
 * command write and the two belong together.
 */
/*
 * IDENTIFY DEVICE: the 512 bytes a drive returns to describe itself.  The
 * guest asks for the capacity, so that is what is filled in - the geometry
 * and the LBA count - and the rest is left zero.
 */
static void
ide_identify(int unit)
{
    unsigned long total;

    total = (unsigned long)cyls[unit] * heads[unit] * spt[unit];
    bzero(buf, SECLEN);
    buf[0] = 0x40;              /* word 0: ATA, fixed drive */
    buf[2] = cyls[unit];        /* word 1: cylinders */
    buf[3] = cyls[unit] >> 8;
    buf[6] = heads[unit];       /* word 3: heads */
    buf[7] = heads[unit] >> 8;
    buf[12] = spt[unit];        /* word 6: sectors per track */
    buf[13] = spt[unit] >> 8;
    buf[99] = 0x02;             /* word 49, bit 9: LBA supported */
    buf[120] = total;           /* words 60-61: total LBA sectors */
    buf[121] = total >> 8;
    buf[122] = total >> 16;
    buf[123] = total >> 24;
}

static void
ide_command(byte cmd)
{
    int unit = drvsel & 1;
    unsigned long lba = ide_lba();

    irq = 1;

    if (ide_unit(unit) < 0) {
        status = SDRDY | SERR;
        return;
    }

    wpos = 0;
    drq = 1;

    switch (cmd) {
    case CREAD:
        writing = 0;
        if (ide_rw(unit, lba, buf, 0) != SECLEN) {
            l("ide: read of lba %lu failed\n", lba);
            drq = 0;
            status = SDRDY | SERR;
            return;
        }
#ifndef NODEBUG
        if (traceflags & trace_bio)
            hexdump(buf, SECLEN);
#endif
        break;
    case CWRITE:
        writing = 1;
        break;
    case CIDENTIFY:
        writing = 0;
        ide_identify(unit);
        break;
    default:
        l("ide: unknown command 0x%x\n", cmd);
        drq = 0;
        status = SDRDY | SERR;
        return;
    }
    status = SDRDY | SDRQ;
}

/*
 * Port C is where a register access happens.  The guest writes it several
 * times per access - address, then address with a strobe, then back - and
 * only the strobed writes mean anything.
 *
 * /CS1 changes which pair of registers the strobe reaches, and it is
 * tested first on both paths because the register number alone cannot tell
 * the two apart: register 6 is drive/head through /CS0 and device control
 * through /CS1, and 7 is status and command through /CS0 and alternate
 * status through /CS1.
 */
static void
wr_ide_portc(portaddr p, byte v)
{
    int reg = C_REG(v);

    if (v & C_RST) {
        ide_reset();
        return;
    }

    if (v & C_RD) {
        /*
         * A read strobe.  The drive drives the bus, and what it puts
         * there is a register, or the next word of a sector.
         */
        if (v & C_CS1) {
            /*
             * Alternate status: the status byte, read without the
             * acknowledgement.  Both polling guests are answered here
             * as well, since neither of them wants INTRQ taken down.
             */
            rdlo = (reg == R_STAT) ? status : tf[reg];
            rdhi = 0;
        } else if (reg == R_DATA && drq && !writing && wpos < NWORD) {
            rdlo = buf[wpos * 2];
            rdhi = buf[wpos * 2 + 1];
            wpos++;
            if (wpos == NWORD) {
                drq = 0;
                status = SDRDY;
            }
        } else if (reg == R_STAT) {
            rdlo = status;
            rdhi = 0;
            /*
             * The acknowledgement.  INTRQ is a level and comes down on
             * this read, which is why a guest that is sleeping on the
             * interrupt has to read status in its handler rather than
             * anywhere it likes.
             */
            irq = 0;
            ide_update_irq();
        } else {
            rdlo = tf[reg];
            rdhi = 0;
        }
        trace(trace_ide, "ide: read reg %d -> %02x\n", reg, rdlo);
        return;
    }

    if (v & C_WR) {
        /*
         * A write strobe, and the byte is whatever was left on port A
         * (with port B above it, for a sector).
         */
        if (v & C_CS1) {
            if (reg == R_CTL) {
                nien = (porta & NIEN) ? 1 : 0;
                trace(trace_ide, "ide: interrupts %s\n",
                    nien ? "cut" : "connected");
            }
            ide_update_irq();
            return;
        }
        if (reg == R_DATA && drq && writing && wpos < NWORD) {
            buf[wpos * 2] = porta;
            buf[wpos * 2 + 1] = portb;
            wpos++;
            if (wpos == NWORD) {
                int unit = drvsel & 1;

                drq = 0;
                status = SDRDY;
                /*
                 * The second interrupt of a write: the sector is in, and
                 * the drive says so.  It is raised here rather than when
                 * the guest next looks, because a guest that is waiting
                 * for it has nothing else to go on.
                 */
                irq = 1;
                if (ide_rw(unit, ide_lba(), buf, 1) != SECLEN) {
                    l("ide: write of lba %lu failed\n", ide_lba());
                    status = SDRDY | SERR;
                }
                ide_update_irq();
#ifndef NODEBUG
                if (traceflags & trace_bio)
                    hexdump(buf, SECLEN);
#endif
            }
        } else if (reg == R_STAT) {
            irq = 0;            /* a command write clears INTRQ too */
            ide_command(porta);
            ide_update_irq();
        } else {
            tf[reg] = porta;
            trace(trace_ide, "ide: write reg %d <- %02x\n", reg, porta);
        }
        return;
    }
}

/*
 * Ports A and B, read.  What they answer was composed by the last strobe:
 * a register byte, or one half of the word the drive is offering.  The
 * guest is in read mode whenever it reads them - the write path puts the
 * byte on port A as an output and never reads it back - so this needs no
 * dependency on the mode word.
 */
static byte
rd_ide_pa(portaddr p)
{
    return (rdlo);
}

static byte
rd_ide_pb(portaddr p)
{
    return (rdhi);
}

static void
wr_ide_pa(portaddr p, byte v)
{
    porta = v;
}

static void
wr_ide_pb(portaddr p, byte v)
{
    portb = v;
}

static void
wr_ide_pctl(portaddr p, byte v)
{
    trace(trace_ide, "ide: mode -> 0x%x\n", v);
    mode = v;
}

/*
 * The drive select latch.  Bit 0 clear is the first drive, set the
 * second; the rest of the byte is not wired to anything.
 */
static void
wr_ide_pdrv(portaddr p, byte v)
{
    drvsel = v & 1;
    trace(trace_ide, "ide: unit -> %d\n", drvsel);
}

static int
ide_init()
{
    register_output(IDE_PA, &wr_ide_pa);
    register_output(IDE_PB, &wr_ide_pb);
    register_output(IDE_PC, &wr_ide_portc);
    register_output(IDE_PCTL, &wr_ide_pctl);
    register_output(IDE_PDRV, &wr_ide_pdrv);
    register_input(IDE_PA, &rd_ide_pa);
    register_input(IDE_PB, &rd_ide_pb);

    mode = PPI_RD;
    ide_reset();
    return (0);
}

static int
ide_setup()
{
#ifndef NODEBUG
    trace_ide = register_trace("ide");
#endif
    return (0);
}

struct driver ide_driver = {
    "ide",
    0,
    &ide_setup,
    &ide_init,
    0
};

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_ide_driver()
{
    register_driver(&ide_driver);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
