/*
 * the multio driver is both for the onboard I/O on the multio card
 * and the wunderbus I/O, which are generally equivalent.
 *
 * differences are small: in group 0, base+1 is a dip switch
 * and base+4 are input and output for a parallel port,
 * base +6 output controls the parallel port
 *
 * this has rtc, serial ports and an interrupt controller, 
 * and some parallel stuff
 */

#include "sim.h"
#include "hwsim.h"
#include "util.h"
#include "pic.h"
#include "vimap.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#ifndef NODEBUG
int trace_multio;
int trace_uart;
int trace_clock;
int trace_intr;
int trace_intack;
#endif

#define HZ_1    0x800
#define NOCLOCK 0x1000

#define BAUDCLOCK   1843200L

/*
 * there's one of these for every ACE.
 */
struct ace {
    char *name;
    int outfd;
    int infd;

    // the chip's registers
    byte rxb;           // rx buffer
    byte txb;           // tx buffer
    byte lcr;           // line control
    byte lsr;           // line status register
    byte mcr;           // modem control character
    byte msr;           // modem status character
    byte dll;           // baud rate lo
    byte dlm;           // baud rate hi
    byte inte;          // interrupt enables
    byte inti;          // interrupt id

    byte int_state;     // these are a latched interrupt cause
#define IS_RX           0x01
#define  IS_TX           0x02

    int vi_line;        // what line to yank on

    // do not deliver characters any faster than the baud rate
    u64 rxwait;         // rx only when
    u64 txpend;         // tx done when
    byte txe_ack;       // THRE interrupt was reset by an IIR read

    int line;
    int want_xterm;     // uart1/uart2: open the xterm on first traffic
    u64 chartime;       // shift time through the uart
} ace[3];

#define TXE_TIMEOUT     5000     // how long to wait before setting LSR_TXE

#define	MULTIO_MASTER	0x48	// multio master standard port
#define	SBASE	        0x40	// the second MultIO's base, one group down

// linestat                 base + 5    0x4d
#define LSR_DR      0x01    // input data ready
#define LSR_OVER    0x02    // input data overrun
#define LSR_PERR    0x04    // input parity error
#define LSR_FERR    0x08    // input framing error
#define LSR_BRK     0x10    // break
#define LSR_TXE     0x20    // transmit buffer empty
#define LSR_TE      0x40    // transmitter empty

// modem control            base + 4    0x4c
#define MCR_DTR     0x01    // data terminal ready
#define MCR_RTS     0x02    // request to send
#define MCR_LOOP    0x10    // loopback

// line control             base + 3    0x4b
#define LCR_82      0x07    // 8 data bits, 2 stop bits
#define LCR_DLAB    0x80    // accessio baud in inte, txb

// interrupt enable         base + 1    0x49
#define INTE_RDAV   0x01    // read available
#define INTE_TXE    0x02    // tx hold empty
#define INTE_RLINE  0x04    // read line status change
#define INTE_MDM    0x08    // modem status change

// interrupt identify       base + 2    0x4a
#define INTI_MDM    0x0     // modem status change
#define INTI_NOINT  0x1     // no interrupt
#define INTI_TXE    0x2     // tx buffer empty
#define INTI_RDAV   0x4     // rx char available
#define INTI_RSTAT  0x6     // reciever status

// modem status
#define MSR_DCTS    0x01
#define MSR_DDSR    0x02
#define MSR_TERI    0x04
#define MSR_DDCD    0x08
#define MSR_CTS     0x10
#define MSR_DSR     0x20
#define MSR_RI      0x40
#define MSR_DCD     0x80

// group select             base + 7    0x4f
#define GROUP_MASK  0x03    
#define MEM_BANK    0x04
#define INTENA      0x08
#define PREST       0x10
#define PAROUT      0x20
#define GRP0        0x00    // pports, clock, pic
#define GRP1        0x01    // serial 1
#define GRP2        0x02    // serial 2
#define GRP3        0x03    // serial 3

// the group select register
static byte group;

char *
printable(char v)
{
    static char sbuf[10];

    if (v >= ' ' && v < 0x7f) {
        sprintf(sbuf, "%c", v);
    } else switch(v) {
    case '\n':  return "\\n";
    case '\t':  return "\\t";
    case '\r':  return "\\r";
    case '\b':  return "\\b";
    case 0:    return "NULL";
    case 26:    return "^Z";
    default:
        sprintf(sbuf, "%02x %d", v, v);
        break; 
    }
    return sbuf;
}

static char *group_bits[] = { 0, 0, "membank", "intena", "printreset", "parstb", 0, 0 };
static char *intbits[] = { "", "", "", "", "", "", "", "" };
static char *inti_bits[] = { "NOINT", "TXE", "RDAV", 0, 0, 0, 0, 0 };
static char *inte_bits[] = { "READAVAIL", "TXHOLDEMPTY", "RLINESTAT", "MDMSTAT", 0, 0, 0, 0 };
static char *lcr_bits[] = { "WLS0", "WLS1", "STB", "PEN", "EPS", "STP", "SBRK", "DLAB" };
char *lsr_bits[] = { "DR" , "OVER", "PERR", "FERR", "BRK", "TXE", "TE", 0 };
static char *mcr_bits[] = { "DTR", "RTS", "OUT1", "OUT2", "LOOP", 0, 0, 0 };
static char *msr_bits[] = { "DCTS", "DDSR", "TERI", "DDCD", "CTS", "DSR", "RI", "DCD" };
static char *wclk_bits[] = { "DATA", "DSTROBE", 0, 0, 0, "CSTROBE", 0, 0 };

#define IRQ0    0x01        // vi0 - hddma/hdca
#define IRQ1    0x02        // vi1 - djdma
#define IRQ2    0x04        // vi2
#define IRQ3    0x08        // uart 1
#define IRQ4    0x10        // uart 2
#define IRQ5    0x20        // uart 3
#define IRQ6    0x40        // daisy
#define IRQ7    0x80        // clock interrupt

/*
 * Two 8259s - the master on the base Wunderbus at 0x48 and the slave on
 * the second MultIO at 0x40 - each addressed at its base+4 (command) and
 * base+5 (data).  The chip model is pic.c, shared with the SS1; what is
 * this board's is the port decode, the cascade and the INT line.
 */
static struct pic pics[2];      /* [0] = master 0x48, [1] = slave 0x40 */

/*
 * Which PIC a register address belongs to.  The two boards are one group
 * apart - the master at 0x48, the slave at 0x40 - so dropping the register
 * bits leaves the base and nothing else has to be known.
 */
static struct pic *
picfor(portaddr p)
{
    return ((p & ~7) == MULTIO_MASTER) ? &pics[0] : &pics[1];
}

void
reg_intbit(int viline, char *name)
{
    intbits[viline] = name;
}

/*
 * the 8259 will send 3 bytes in response to an interrupt ack.
 * the first is a 0xcd, then the high, then the low bytes of the service routine
 * we have a 0xff as the first byte, which is a broken intack vector.
 */
unsigned char vector[3];
#define IV_FILLED   0
#define IV_1SENT    1
#define IV_2SENT    2
#define IV_EMPTY    3
#define IV_INVALID  4

int ivecstate = IV_INVALID;
char *ivs_name[] = { "filled", "1", "2", "empty", "invalid" };
unsigned char iv_isrbit;

int
multio_dump(char **p)
{
    struct pic *m = &pics[0];
    struct pic *s = &pics[1];

    l("multio dump: group %d\n", group);

    /*
     * The master's registers, which is what the CPU programs; the slave
     * is only ever polled, so its irr is the one thing worth showing.
     */
    l("\tpic_state: %d\n", m->pic_state);
    l("\ticw1: %02x\n", m->icw1);
    l("\ticw2: %02x\n", m->icw2);
    l("\ticw3: %02x\n", m->icw3);
    l("\ticw4: %02x\n", m->icw4);
    l("\tocw2: %02x\n", m->ocw2);
    l("\tocw3: %02x\n", m->ocw3);
    l("\timr: %02x %s\n", (~m->imr) & 0xff, bitdef(~m->imr, intbits));
    l("\tirr: %02x %s\n", m->irr, bitdef(m->irr, intbits));
    l("\tisr: %02x %s\n", m->isr, bitdef(m->isr, intbits));
    l("\tvecbase: %04x\n", (m->icw1 & ICW1_VECL) + (m->icw2 << 8));
    l("\tpriority: %d %02x\n", m->priority, 1 << m->priority);
    l("\tslave irr: %02x isr: %02x\n", s->irr, s->isr);
    l("\tint_line: %d int_pin: %d vi_lines %02x\n", int_line, int_pin, vi_lines);
    l("\tivecstate: %s isrbit: %02x vector %02x %02x %02x\n",
        ivs_name[ivecstate], iv_isrbit, vector[0], vector[1], vector[2]);
    return 0;
}

struct ace *
select_ace()
{
    int line = group & GROUP_MASK;
    
    if (line) {
        line--;
    } else {
        l("select_ace: group 0\n");
    }
    return &ace[line];
}

static void multio_set_int_line();

/*
 * The board's four register accesses: the master's are reached through
 * the group-select latch (GRP0 puts the master's command/data at 0x4c
 * and 0x4d), the slave's are wired straight to its own base and are
 * always GRP0.  Which PIC the access belongs to is the address alone.
 */
static void
wr_pic_port_0(portaddr p, byte v)
{
    pic_write_cmd(picfor(p), v);
}

static void
wr_pic_port_1(portaddr p, byte v)
{
    pic_write_data(picfor(p), v);
}

static byte
rd_pic_port_0(portaddr p)
{
    return pic_read_cmd(picfor(p));
}

static byte
rd_pic_port_1(portaddr p)
{
    return pic_read_data(picfor(p));
}

/*
 * based on the irr, imr and isr, set the s100 int line
 */
void
multio_set_int_line()
{
    int line;

    /*
     * Only the active interrupt controller folds the INT line.  When the
     * SS1 is the int_line driver this board's uarts and clock still poll,
     * but they must not yank int_change - the SS1 owns it.
     */
    if (!wiring_active("multio"))
        return;

    /*
     * PINT is wired-OR across the two PICs, but only the master owns
     * SINTA, so the slave's request has to reach the master as one of its
     * own inputs to be vectored.  The slave's INT is wired to the
     * master's cascade line (SLAVE_CASCADE_IR): ask the slave first, then
     * fold its answer into the master's irr, so the master's priority
     * resolver sees the cascade line when the slave fires.
     */
    line = (pic_serviceable(&pics[1]) >= 0) ? 1 : 0;

    if (line)
        pics[0].irr |= (1 << SLAVE_CASCADE_IR);
    else
        pics[0].irr &= ~(1 << SLAVE_CASCADE_IR);

    /* the master's own resolver now covers both PICs */
    line = (pic_serviceable(&pics[0]) >= 0) ? 1 : 0;
    trace(trace_multio, "multio: %s line irr:%02x isr:%02x pri:%d\n",
        line ? "set":"clear", pics[0].irr, pics[0].isr, pics[0].priority);
    (*int_change)(line);
}

/*
 * Drive one of the master's on-board inputs directly: the serial ports
 * sit on the master's IR3..IR5 and the clock on IR7, and none of those
 * are S-100 VI lines, so they do not pass through set_vi/vi_lines.  The
 * disk controllers are the only thing on the bus's VI lines.
 */
void
multio_master_ir(int ir, int value)
{
    /*
     * The board's own PIC is only the machine's when it is the wired
     * controller.  Dormant beside the SS1 it still has ACEs and their
     * registers may be read and written, but nothing here may fold an
     * INT line the SS1 owns.
     */
    if (!wiring_active("multio"))
        return;
    pic_request(&pics[0], ir, value);
    multio_set_int_line();
}

/*
 * The S-100 vectored-interrupt lines changed (set_vi).  vi_lines is the
 * eight VI lines, and bit n is VI n.  Only the disk controllers live
 * here: VI0..VI2 are the master's IR0..IR2, VI3..VI5 the slave's IR0..IR2
 * (shifted down), and VI6..VI7 are not wired to a card.  The serial ports
 * and clock are not bus lines and reach the master directly through
 * multio_master_ir.
 */
void
multio_vi_change(unsigned char new)
{
    byte mask = vi_lines ^ new;

    trace(trace_multio, "multio: vi_changed lines:%x new:%x diff:%x\n",
            vi_lines, new, mask);

    vi_lines = new;

    /*
     * Level, not edge: set each VI input's IRR bit from the line's
     * current level.  A change-only update loses a request that was
     * acknowledged while the card kept the line high (the NIC does
     * exactly that with a frame still in its FIFO).  VI2 on the master
     * is the slave's cascade, below, and is not a VI input.
     */
    pics[0].irr = (pics[0].irr & ~MASTER_VI_MASK) |
        (vi_lines & MASTER_VI_MASK);
    pics[1].irr = (pics[1].irr & ~0x07) |
        ((vi_lines & SLAVE_VI_MASK) >> 3);

    multio_set_int_line();
}

/*
 * return a byte of interrupt acknowledge. called by the chip simulator
 * this needs to implement the 8259 state machine
 * when sending the last byte of the vector, we set the isr
 * for that vector.
 */
unsigned char
multio_intack()
{
    int level;
    byte mask;
    int vecaddr;
    struct pic *p;              /* the PIC the vector came from */
    unsigned char i;

    /* only the master owns SINTA; the slave is polled, never vectored */
    p = &pics[0];

    trace(trace_intack, "multio_intack: ivecstate=%d %s\n",
        ivecstate, ivs_name[ivecstate]);

    // the vector is empty, so let's calculate it
    if (ivecstate == IV_EMPTY) {
        /*
         * Select the highest-priority pending request that is unmasked
         * and higher priority than the highest-priority in-service level,
         * exactly as the 8259 priority resolver does.
         */
        level = pic_serviceable(&pics[0]);

        trace(trace_intack, "intack: irr:%02x isr:%02x pri:%d lvl:%d\n",
            pics[0].irr, pics[0].isr, pics[0].priority, level);

        /*
         * The CAS cascade: when the master's cascade line (IR6) is the
         * request and the slave has been given a vector base (its ICW2 is
         * non-zero), the slave drives its own vector instead of the
         * master's.  A polled slave's ICW2 is zero, so the master vectors
         * IR6 as usual and ovlint6 polls the slave.  Neither in-service
         * bit is set here - the master's EOI is a no-op and the slave's
         * line is level-triggered, its irr cleared below and re-armed by
         * the next line edge - so the dismiss needs nothing beyond the
         * master EOI.
         */
        if (level == SLAVE_CASCADE_IR) {
            if (pics[1].icw2 != 0) {            /* the slave has a vector base */
                pics[0].irr &= ~(1 << SLAVE_CASCADE_IR); /* master's cascade */
                iv_isrbit = 0;
                p = &pics[1];
                level = pic_serviceable(p);     /* the slave's line */
            }                                   /* else polled: master IR6 */
        }

        // we didn't find a cause for our interrupt
        if (level < 0) {
            l("lose: no unmasked request found!\n");
            multio_dump(0);
            ivecstate = IV_INVALID;
            iv_isrbit = 0;
        } else {
            mask = 1 << level;
            if (p == &pics[1])
                iv_isrbit = 0;                  /* no slave isr bit to set */
            else
                iv_isrbit = mask;
            /*
             * A level-triggered 8259 clears the request bit when it
             * acknowledges, and the IRQ line has to fall and rise again to
             * set it back.  Keeping irr set here meant a device that left
             * its line high - the hddma holds it from completion until the
             * next command - re-asserted the interrupt the moment the EOI
             * cleared isr, and the kernel took it as a second, spurious
             * interrupt.  Drop the request now; the line's own low/high
             * edges in multio_vi_change will re-raise it for real.
             */
            p->irr &= ~mask;
            vecaddr = pic_vector(p, level);
            vector[0] = 0xcd;
            vector[1] = vecaddr & 0xff;
            vector[2] = (vecaddr >> 8) & 0xff;
            ivecstate = IV_FILLED;
            trace(trace_multio, "multio: vector %d (%s) %04x\n",
                    level, intbits[level], vecaddr);
            trace(trace_intr, "vectoring to %02x %s handler at 0x%04x\n",
                    mask, intbits[level], vecaddr);
        }
    }

    switch (ivecstate) {
    case IV_INVALID:
        ivecstate = IV_EMPTY;
        i = 0xff;
        break;
    case IV_FILLED:
        ivecstate++;
        i = vector[0];
        break;
    case IV_1SENT:
        ivecstate++;
        i =  vector[1];
        break;
    case IV_2SENT:
        ivecstate = IV_EMPTY;
        i =  vector[2];
        break;
    default:
        l("bad ivecstate %d\n", ivecstate);
        i =  0xff;
        iv_isrbit = 0;
        ivecstate = IV_EMPTY;
        break;
    }

    // if we just sent the last ivec byte, set the isr bit
    if (ivecstate == IV_EMPTY) {
        if (p->auto_eoi) {
            /* auto-eoi: the 8259 drops the in-service bit on its own
             * and, with rotation, demotes the serviced level to lowest */
            if (p->rotate && iv_isrbit) {
                int lvl;

                for (lvl = 0; lvl < 8; lvl++)
                    if (iv_isrbit & (1 << lvl))
                        break;
                p->priority = (lvl + 1) % 8;
            }
        } else {
            p->isr |= iv_isrbit;
        }
        // clear the interrupt if there are no more enabled
        multio_set_int_line();
    }
    return i;
}

/*
 * manage the inti register, and the corresponding vi line
 * this is based on the conditions of the enables and the
 * lsr
 */
void
multio_set_inti(struct ace *ap)
{
    unsigned char before = ap->inti;

    if ((ap->inte & INTE_RDAV) && (ap->lsr & LSR_DR)) {
        ap->inti = INTI_RDAV;
    } else if ((ap->inte & INTE_TXE) && (ap->lsr & LSR_TXE) && !ap->txe_ack) {
        ap->inti = INTI_TXE;
    } else {
        ap->inti = INTI_NOINT;
    }

    /*
     * The 8259 clears the request bit when it acknowledges (see
     * multio_intack) and re-arms it only on a fresh line edge.  So the
     * line must edge on every change of cause, not only when the
     * interrupt goes away entirely: a still-pending TXE that loses the
     * IIR slot to a higher-priority RDAV changes the cause TXE->RDAV and
     * later RDAV->TXE without ever touching INTI_NOINT.  Without an edge
     * there, the second cause is never re-latched and its wakeup is
     * dropped - the output channel stalls, and the next receive is lost
     * with it.
     */
    if (before != ap->inti) {
        multio_master_ir(ap->vi_line, 0);
        if (ap->inti != INTI_NOINT)
            multio_master_ir(ap->vi_line, 1);
    }
}

/*
 * the uart registers hold truth all the time.   any dynamic changes to the registers,
 * like txready, rxready, etc, are updated by a poll. all interrupts are generated from the poll
 */
int
multio_uart_poll(struct ace *ap)
{
    int bytes;
    int error;
    long long t;

    t = simnow64();

    // if we are shifting a character out.
    if (ap->txpend) {
        // has it been shifted out yet?
        if (t >= ap->txpend) {
            ap->lsr |= LSR_TXE;
            ap->txpend = 0;
            multio_set_inti(ap);

            // if we are in loopback mode, the output character now appears in the input register
            if (ap->mcr & MCR_LOOP) {
                ap->rxb = ap->txb;
                ap->lsr |= LSR_TXE | LSR_DR;
                multio_set_inti(ap);
            }
        }
    }
    
    // let's not try to read from a non-connected terminal
    if (ap->infd == -1) {
        return 0;
    }

    /*
     * Uart 0 shares its descriptor with the djdma's serial port, and a
     * character can only be delivered once.  If the controller has the
     * console, leave the input alone - it is not ours.  The other two
     * uarts have descriptors of their own and are never in question.
     *
     * That sharing is only the wired controller's: beside the SS1 this
     * board's first ACE has its own window, and that window's input must
     * be read even though the console claim belongs to the other board.
     */
    if (ap == &ace[0] && wiring_active("multio") &&
        console_owner != CONS_MULTIO) {
        return 0;
    }

    // let's not recieve characters any faster than the recieve baud rate
    if (t < ap->rxwait) {
#ifdef notdef
        trace(trace_uart, "early poll ret %lld %lld %lld\n", 
            t, ap->rxwait, ap->rxwait - t);
#endif
        return 0;
    }

    // let's not do a reciever overrun
    if (ap->lsr & LSR_DR) {
#ifdef notdef
        trace(trace_uart, "rx overrun\n", t, ap->rxwait);
#endif
        return 0;
    }

    // test to see if there are any input characters
    if ((error = ioctl(ap->infd, FIONREAD, &bytes)) != 0) {
        l("multio_uart_poll: FIONREAD ioctl failed: %d\n",
            error);
        return 0;
    }

    // got any?
    if (bytes == 0) {
        return 0;
    }

    // lets get one
    if ((bytes = read(ap->infd, &ap->rxb, 1)) != 1) {
        l("multio_uart_poll: read failed: %d\n",
            bytes);
        return 0;
    }

    ap->lsr |= LSR_DR;
    
    // and let's let the simulator know
    if (ap->inte & INTE_RDAV) {
        multio_set_inti(ap);
    }
    return 0;
}

static void
ace_init(int i, char *name, int vi_line)
{
    struct ace *ap = &ace[i];

    ap->msr = MSR_CTS | MSR_DSR /* | MSR_RI */ | MSR_DCD;
    ap->lsr = LSR_TXE;
    ap->name = name;
    ap->vi_line = vi_line;
    ap->rxwait = simnow64();
    reg_intbit(vi_line, ap->name);
    ap->infd = -1;
    ap->outfd = -1;
    ap->line = i;
}

/*
 * recieve buffer.  
 * if LCR_DLAB is set, access to lsb of baud rate divisor
 */
static byte 
rd_rxb(portaddr p) 
{
    struct ace *ap = select_ace();
    unsigned char retval;

    if (ap->lcr & LCR_DLAB) {
        return ap->dll;
    }

    // only read valid data if DR, else return null */
    if (ap->lsr & LSR_DR) {
        retval = ap->rxb;

        ap->lsr &= ~LSR_DR;
        ap->rxwait = simnow64() + ap->chartime;
        multio_set_inti(ap);
    } else {
        retval = 0;
    }
    trace(trace_uart, "%s: read rxb = %s%s\n",
         ap->name, ap->mcr & MCR_LOOP ? "(loopback) " : "", printable(retval));
    return retval;
}

/*
 * Lazily open the xterm for a non-console uart.  Called from wr_txb the
 * first time a character is actually transmitted - the login prompt -
 * not at startup.  multio_init leaves want_xterm set on uart1/uart2
 * instead of opening their windows up front.
 */
static void
lazy_open_xterm(struct ace *ap)
{
    char tname[10];

    sprintf(tname, "multio-%d", ap->line);
    open_terminal(tname, 0, &ap->infd, &ap->outfd, 0, 0);
    ap->want_xterm = 0;
}

/*
 * transmitter buffer
 * if LCR_DLAB is set, access to lsb of baud rate divisor
 */
static void
wr_txb(portaddr p, byte v)
{
    struct ace *ap = select_ace();

    trace(trace_uart, "multio: txb %02x %s reaches the uart, group %d\n",
        v, printable(v), group);

    /*
     * Sending on uart 0 claims the console back from the floppy
     * controller, if it ever had it.  Normally this changes nothing -
     * the multio holds the console from reset - and it costs a compare.
     */
    if (ap == &ace[0]) {
        console_claim(CONS_MULTIO);
    }

    if (ap->lcr & LCR_DLAB) {
        trace(trace_uart, "%s: write dll %02x\n", ap->name, v);
        ap->dll = v;
        return;
    }

    // detect overrun
    if (!(ap->lsr & LSR_TXE)) {
        l("multio: send overrun on line %d\n", ap->line);
    }

    ap->txpend = simnow64() + ap->chartime;
    ap->txb = v;
    ap->lsr &= ~LSR_TXE;
    ap->txe_ack = 0;        // this character will empty the register again

    /*
     * Tee the console to the log file independent of the terminal fd: the
     * -L log must keep working even when uart0 has no xterm (outfd == -1),
     * otherwise a headless run loses all its console output.  uart 0 is
     * the console here; the SS1's and the floppy controller's consoles
     * call the same console_log(), so -L follows the wiring.
     */
    if ((ap->mcr & MCR_LOOP) == 0) {
        /*
         * The first character actually delivered to a non-console uart
         * opens its xterm, so the window appears with the login prompt
         * in it.  Only writes that reach the line count: the baud-rate
         * setup goes by under DLAB, and the boot rom probes every uart
         * by putting it in loopback and sending two nulls, neither of
         * which is traffic on the terminal.
         */
        if (ap->want_xterm && ap->outfd == -1)
            lazy_open_xterm(ap);
        if (ap->outfd != -1)
            write(ap->outfd, &v, 1);
        if (ap == &ace[0])
            console_log(v);
    }
    trace(trace_uart, "%s: write txb %s%s\n", 
        ap->name, ap->mcr & MCR_LOOP ? "(loopback) " : "", printable(v));
}

/*
 * interrupt enable register
 * if LCR_DLAB is set, access to msb of baud rate divisor
 */
static byte 
rd_inte(portaddr p)
{
    struct ace *ap = select_ace();

    if (ap->lcr & LCR_DLAB) {
        return ap->dlm;
    }
    trace(trace_uart, "%s: read inte %02x %s\n", 
        ap->name, ap->inte, bitdef(ap->inte, inte_bits));
    return ap->inte;
}

static void
wr_inte(portaddr p, byte v)
{
    struct ace *ap = select_ace();

    if (ap->lcr & LCR_DLAB) {
        trace(trace_uart, "%s: write dlm %02x\n", ap->name, v);
        ap->dlm = v;
        return;
    }
    /*
     * Enabling the transmit interrupt while the holding register is
     * already empty raises it.  The 8250 resets THRE on an iir read, but
     * that reset lasts only until the condition is asserted again - and
     * writing the enable bit with the register empty asserts it.
     *
     * Without this the only way to ever see another transmit interrupt
     * is to write a character, and the kernel's way of starting output
     * on an idle line is the other way round: queue the character, arm
     * the interrupt, wait.  So output flows for as long as it is
     * continuous, because each completion pulls the next character out,
     * and stops dead at the first character written after the queue has
     * once run empty.  That is the shell's prompt: the whole of the
     * welcome text arrives and the "# " after it never does.
     *
     * Same shape as the djdma completion that went missing - a level
     * that is not re-examined when the thing that would raise it
     * changes.
     */
    if ((v & INTE_TXE) && (ap->lsr & LSR_TXE)) {
        ap->txe_ack = 0;
    }

    ap->inte = v;
    trace(trace_uart, "%s: write inte %02x %s\n",
        ap->name, ap->inte, bitdef(ap->inte, inte_bits));
    multio_set_inti(ap);
}

/*
 * line control register
 * contains the divisor access bit that changes the meaning of txb,rxb,and inte
 */
static byte 
rd_lcr(portaddr p)
{
    struct ace *ap = select_ace();

    trace(trace_uart, "%s: read lcr %02x %s\n", 
        ap->name, ap->lcr, bitdef(ap->lcr, lcr_bits));
    return ap->lcr;
}

static void
wr_lcr(portaddr p, byte v)
{
    struct ace *ap = select_ace();

    // when resetting LCR_DLAB, calculate the character shift time
    if ((ap->lcr & LCR_DLAB) && !(v & LCR_DLAB)) {
        ap->chartime = (((ap->dlm << 8) + ap->dll) * 16 * 1000000L * 10);
        ap->chartime /= BAUDCLOCK;
        trace(trace_uart, "%s: set baud rate %d %lld\n", 
            ap->name, (ap->dlm << 8) + ap->dll, ap->chartime);
    }
    ap->lcr = v;
    trace(trace_uart, "%s: write lcr %02x %s\n", 
        ap->name, ap->lcr, bitdef(ap->lcr, lcr_bits));
}

/*
 * line status register.
 */
static byte 
rd_lsr(portaddr p)
{
    struct ace *ap = select_ace();
    static unsigned char last[3];
    // if it changed, maybe log it
    if (ap->lsr != last[ap->line]) {
        trace(trace_uart, "%s: read linestat %02x %s\n", 
            ap->name, ap->lsr, bitdef(ap->lsr, lsr_bits));
    }
    last[ap->line] = ap->lsr;
    return ap->lsr;
}

// the chip docs say this register is not well defined for write.
static void
wr_lsr(portaddr p, byte v)
{
    struct ace *ap = select_ace();

    trace(trace_uart, "%s: write linestat %02x %s\n", 
        ap->name, v, bitdef(v, lsr_bits));
}

/*
 * modem control register.
 * contains the loopback enable
 */
static byte 
rd_mcr(portaddr p)
{
    struct ace *ap = select_ace();

    trace(trace_uart, "%s: read mdmctl %02x %s\n", 
        ap->name, ap->mcr, bitdef(ap->mcr, mcr_bits));
    return ap->mcr;
}

static void
wr_mcr(portaddr p, byte v)
{
    struct ace *ap = select_ace();

    ap->mcr = v;
    trace(trace_uart, "%s: write mdmctl %02x %s\n", 
        ap->name, ap->mcr, bitdef(ap->mcr, mcr_bits));
}

/*
 * the interrupt identify register indicates which interrupt causes are pending
 * each of the 4 interrupts are reset differently: (in order)
 * 110 - reciever line error or break  - read the lsr
 * 100 - reciever data available - read the rxd
 * 010 - transmitter buffer empty - writing the tx or reading this cause from inti
 * 000 - CTS, DSR, RI, DCD - read msr
 */
static byte 
rd_inti(portaddr p)
{
    struct ace *ap = select_ace();

    byte retval = ap->inti;

    trace(trace_uart, "%s: read inti %02x %s\n", 
        ap->name, retval, bitdef(retval, inti_bits));

    /*
     * Table IV of the 8250A data sheet: the transmitter holding register
     * empty interrupt is reset by "Reading the IIR Register (if source of
     * interrupt) or Writing into the Transmitter Holding Register".
     *
     * That reset has to stick.  THRE is not a level that can be sampled
     * again the moment anyone asks - the condition is still true, the
     * register is still empty - it stays reset until the holding register
     * empties once more, which only a write can bring about.  Re-deriving
     * it from the condition made the driver spin: it read the IIR, got
     * THRE, re-enabled the interrupt, and wr_inte handed it THRE straight
     * back, seven thousand times, without ever sending a character.
     */
    if (ap->inti == INTI_TXE) {
        ap->inti = INTI_NOINT;
        ap->txe_ack = 1;
        multio_master_ir(ap->vi_line, 0);
    }
    /*
     * The read reports the interrupt that WAS pending and resets it as a
     * side effect; it does not report the state after its own reset.
     * Returning ap->inti here handed the driver "no interrupt pending"
     * every time, so it could never work out what to service.
     */
    return retval;
}

static byte 
rd_msr(portaddr p)
{
    struct ace *ap = select_ace();

    trace(trace_uart, "%s: read mdmstat %02x %s\n", 
        ap->name, ap->msr, bitdef(ap->msr, msr_bits));
    return ap->msr;
}

// it is unclear from the chip doc what happens when you write this register
static void 
wr_msr(portaddr p, byte v)
{
    struct ace *ap = select_ace();

    ap->msr = v;

    trace(trace_uart, "%s: write mdmstat %02x %s\n", 
        ap->name, ap->msr, bitdef(ap->msr, msr_bits));
}

/*
 * the clock is bit-banged, a 1990 clock/calendar chip.
 */
#define CLK_DATA    0x01    // the data shifts in and out here
#define CLK_SHIFT   0x02    // the shift register strobe
#define CLK_CMD     0x1c    // command bits
#define     CC_SHOLD    0x00    // shift register hold
#define     CC_ENSR     0x04    // enable shift register
#define     CC_SET      0x08    // load clock from shift register
#define     CC_GET      0x0c    // read clock to shift register
#define     CC_64HZ     0x10
#define     CC_256HZ    0x14
#define     CC_2kKHZ    0x18
#define     CC_32HZ     0x1c    // really test mode
#define CLK_SETCMD  0x20    // strobe the command

#define  bcd(h,l)    ((((h) & 0xf) << 4) | ((l) & 0xf))

int clock_happened;

void
clock_handler()
{
    clock_happened = 1;
}

/*
 * the real time clock is latched this 40 bit structure
 */
byte rtc[5] = { 0x25, 0x34, 0x12, 0x25, 0x76 };
int rtcptr;
static byte wrclock[5];             /* shift register being written (CC_SET) */
static int wrclockptr;
static int wrclock_dirty;           /* a bit was shifted since the last ENSR/SET */
/*
 * The clock the guest reads is UTC, not the host's local time.
 *
 * Micronix has no timezone - it is v6 underneath, and its timestamps
 * are plain counts of seconds from 1970 - so a file's time means
 * whatever frame the clock it was written with was in.  Everything
 * that reaches a volume from outside, though, carries a UTC epoch: the
 * host tools store st_mtime as it comes off the host, and "mnix tar"
 * puts a source file's mtime on disk untouched.  Read the RTC as local
 * time and the guest's clock lands one offset away from every file
 * installed that way, which is fatal to make: the sources are ahead of
 * the clock that stamps the objects built from them, so every target
 * is remade on every run and never converges.  Reading UTC puts the
 * guest's clock in the frame its own files are already in.
 *
 * The offset then means what it looks like: the guest's own error,
 * after it has set the clock, against real time.  CC_SET has to build
 * its epoch the same way round, hence timegm below and not mktime.
 */
static time_t rtc_offset;           /* rtc = host time + this offset */
static byte last_wrclock;
time_t nowtime;
char *clk_cmd[] = { "SHOLD", "ENSR", "SET", "GET", "64Hz", "256Hz", "2kHz", "32Hz" };

/*
 * the bit banging works like this:
 * the program sets strobe low, with command set, 
 * then it sets it high, again with the same command,
 * then it sets it low again
 * I'm going to be lazy and only notice the high to low transitions
 */
static void
wr_clock(portaddr p, byte v)
{
    struct tm *tm;
    int rate = 0;

    trace(trace_multio, "multio: write clock %02x %s %s\n", 
        v, clk_cmd[(v & CLK_CMD) >> 2], bitdef(v, wclk_bits));

    // if falling edge on CSTROBE and command the same, do a command
    if ((!(v & CLK_SETCMD)) && (last_wrclock & CLK_SETCMD) &&
        ((v & CLK_CMD) == (last_wrclock & CLK_CMD))) {
        switch (v & CLK_CMD) {
        case CC_SHOLD:      // do nothing with shift register
            break;
        case CC_ENSR:       // reset shift register
            rtcptr = 0;
            wrclockptr = 0;
            wrclock_dirty = 0;
            break;
        case CC_SET:        // set time: shift register -> clock
        {
            /*
             * The real 1990's "time set & counter hold" command only loads
             * the shift register when data has been shifted in; the kernel's
             * _minit() strokes it with an empty register just to hold the
             * counter.  Honour that: with nothing shifted, do not touch the
             * time offset.
             */
            if (wrclock_dirty) {
                struct tm st = { 0 };
                struct tm *now;
                time_t t = time(0);

                now = gmtime(&t);
                st.tm_year = now->tm_year;  /* the 40-bit rtc has no year */
                st.tm_mon  = (wrclock[4] >> 4) & 0xf;
                st.tm_mday = ((wrclock[3] >> 4) & 0xf) * 10 + (wrclock[3] & 0xf);
                st.tm_hour = ((wrclock[2] >> 4) & 0xf) * 10 + (wrclock[2] & 0xf);
                st.tm_min  = ((wrclock[1] >> 4) & 0xf) * 10 + (wrclock[1] & 0xf);
                st.tm_sec  = ((wrclock[0] >> 4) & 0xf) * 10 + (wrclock[0] & 0xf);
                st.tm_isdst = -1;
                rtc_offset = timegm(&st) - t;
#ifndef NODEBUG
                fprintf(stderr, "CC_SET: wr=%02x %02x %02x %02x %02x off=%ld\n",
                    wrclock[0], wrclock[1], wrclock[2], wrclock[3], wrclock[4], (long)rtc_offset);
#endif
            }
            wrclock_dirty = 0;
            break;
        }
        case CC_GET:        // read the clock and populate the array
            nowtime = time(0) + rtc_offset;
            tm = gmtime(&nowtime);
            rtc[0] = bcd(tm->tm_sec / 10, tm->tm_sec % 10);
            rtc[1] = bcd(tm->tm_min / 10, tm->tm_min % 10);
            rtc[2] = bcd(tm->tm_hour / 10, tm->tm_hour % 10);
            rtc[3] = bcd(tm->tm_mday / 10, tm->tm_mday % 10);
            rtc[4] = bcd(tm->tm_mon, tm->tm_wday);
            break;
        case CC_64HZ:
            rate = 64;
            break;
        case CC_256HZ:
            rate = 256;
            break;
        case CC_2kKHZ:
            rate = 2048;
            break;
        case CC_32HZ:           // test mode in the real chip. tricky.
            rate = 32;
            break;
        }
        if ((config_sw & CONF_SET) && (config_sw & HZ_1)) {
            rate = 1;
        }
        /*
         * Whether there is a clock is a property of the machine, not of
         * what you are watching, so it hangs off the configuration
         * switches and not off the trace flags.  It used to be trace bit
         * 800, "noclock", which meant that asking for every trace with
         * -t 0xfffff turned the clock off - and a Micronix with no clock
         * does not fail, it spins in the scheduler looking for something
         * to run, forever and silently.  The most detailed trace
         * available was the one that broke the machine.
         */
        if (rate != 0) {
            if (!((config_sw & CONF_SET) && (config_sw & NOCLOCK))) {
                recurring_time_out("multio_clock", rate, clock_handler, 0);
            }
        }
    }

    // if falling edge on DSTROBE and command the same, shift a bit
    if ((!(v & CLK_SHIFT)) && (last_wrclock & CLK_SHIFT) &&
        ((v & CLK_CMD) == (last_wrclock & CLK_CMD))) {
        /* the data pin shifts into the write register, lsb first */
        if (v & 1)
            wrclock[wrclockptr / 8] |= 1 << (wrclockptr % 8);
        else
            wrclock[wrclockptr / 8] &= ~(1 << (wrclockptr % 8));
        wrclock_dirty = 1;
        if (++rtcptr == 40)
            rtcptr = 0;
        if (++wrclockptr == 40)
            wrclockptr = 0;
    }
    last_wrclock = v;
}

static byte 
rd_clock(portaddr p)
{
    byte v;

    v = (rtc[rtcptr / 8] >> (rtcptr % 8)) & 1;
    trace(trace_multio, "multio: read clock %02x\n", v);

    multio_master_ir(7, 0);

    return v;
}

static byte
rd_daisy(portaddr p)
{
    byte ret;

    ret = 0;
    trace(trace_multio, "multio: read daisy %02x = 0x%02x\n", p, ret);
    return ret;
}

static void
wr_daisy0(portaddr p, byte v)
{
    trace(trace_multio, "multio: write daisy0 %02x\n", v);
}

static void
wr_daisy1(portaddr p, byte v)
{
    trace(trace_multio, "multio: write daisy1 %02x\n", v);
}

byte
rd_switch(portaddr p)
{
    return 0xff;
}

static byte
undef_inreg(portaddr p)
{
    trace(trace_multio, "multio: read to %02x+%02x undefined\n", 
        MULTIO_MASTER, p - MULTIO_MASTER);
    return 0;
}

static void
undef_outreg(portaddr p, byte v)
{
    trace(trace_multio, "multio: write 0x%02x to undefined %02x+%02x\n", 
        v, MULTIO_MASTER, p - MULTIO_MASTER);
}

/*
 * write the port select register
 */
void
multio_select(portaddr p, byte v)
{
    static int lastgroup = -1;

    /*
     * Which group is selected decides what the eight ports at 0048 mean,
     * so a wrong decode here does not fail, it silently sends characters
     * to the parallel port instead of the uart.  Say what was written
     * and what it was taken to mean.
     */
    trace(trace_multio, "multio: select %02x -> group %d\n",
        v, v & GROUP_MASK);

    group = v & GROUP_MASK;

    if (group == lastgroup)
        return;

    lastgroup = group;

    trace(trace_multio, "multio: write group select %02x %s\n", 
        group, bitdef(v, group_bits));

    switch (group) {
    case GRP0:  // parallel port, clock, pic
        register_input(MULTIO_MASTER + 0, &rd_daisy);
        register_input(MULTIO_MASTER + 1, &rd_switch);
        register_input(MULTIO_MASTER + 2, &rd_clock);
        register_input(MULTIO_MASTER + 3, &undef_inreg);
        register_input(MULTIO_MASTER + 4, &rd_pic_port_0);
        register_input(MULTIO_MASTER + 5, &rd_pic_port_1);
        register_input(MULTIO_MASTER + 6, &undef_inreg);

        register_output(MULTIO_MASTER + 0, &wr_daisy0);
        register_output(MULTIO_MASTER + 1, &wr_daisy1);
        register_output(MULTIO_MASTER + 2, &wr_clock);
        register_output(MULTIO_MASTER + 3, &undef_outreg);
        register_output(MULTIO_MASTER + 4, &wr_pic_port_0);
        register_output(MULTIO_MASTER + 5, &wr_pic_port_1);
        register_output(MULTIO_MASTER + 6, &undef_outreg);
        break; 
    case GRP1:  // serial ports
    case GRP2:
    case GRP3:
        register_input(MULTIO_MASTER + 0, &rd_rxb);
        register_input(MULTIO_MASTER + 1, &rd_inte);
        register_input(MULTIO_MASTER + 2, &rd_inti);
        register_input(MULTIO_MASTER + 3, &rd_lcr);
        register_input(MULTIO_MASTER + 4, &rd_mcr);
        register_input(MULTIO_MASTER + 5, &rd_lsr);
        register_input(MULTIO_MASTER + 6, &rd_msr);

        register_output(MULTIO_MASTER + 0, &wr_txb);
        register_output(MULTIO_MASTER + 1, &wr_inte);
        register_output(MULTIO_MASTER + 2, &undef_outreg);
        register_output(MULTIO_MASTER + 3, &wr_lcr);
        register_output(MULTIO_MASTER + 4, &wr_mcr);
        register_output(MULTIO_MASTER + 5, &wr_lsr);
        register_output(MULTIO_MASTER + 6, &wr_msr);
        break;
    }
}

/*
 * The slave MultIO's group select (0x47).  Its PIC registers are wired
 * up once at init, so a select is accepted and nothing is switched - the
 * slave has no serial ports here to select between, only its PIC.
 */
static void
slave_select(portaddr p, byte v)
{
    trace(trace_multio, "multio: slave select %02x\n", v);
}

int terminal_fd_in;
int terminal_fd_out;

int termfd = -1;
char *myttyname;

static struct termios original_tio;

void
multio_restore_terminal()
{
    if (termfd >= 0)
        tcsetattr(termfd, TCSANOW, &original_tio);
}

/*
 * between instructions, let's check if anything has happened that we need to react
 * to.
 */
int
multio_poll()
{
    int i;
 
    for (i = 0; i < 3; i++) {
        multio_uart_poll(&ace[i]);
    }

    if (clock_happened) {
        clock_happened = 0;
        /*
         * This used to print unconditionally, which put a line between
         * every pair of lines in every trace at whatever rate the clock
         * was running.  It is worth seeing when the question is the
         * clock and worth nothing otherwise, so it costs a bit now - the
         * one that used to turn the clock off, which now lives with the
         * configuration switches where behaviour belongs.
         */
        trace(trace_clock, " ---- clock --- \n");
        multio_master_ir(7, 1);
    }
    return 0;
}

/*
 * Dump the uart register state, for SIGUSR2 / kdump.  This is the sim's
 * view of the ACEs - the interrupt enables and line status - which the
 * guest's tty struct does not record and a wedged output line cannot be
 * diagnosed without.
 */
void
multio_ace_dump(void)
{
    int i;

    for (i = 0; i < 3; i++) {
        struct ace *ap = &ace[i];

        printf(" uart%d %-8s inte=%02x(RDAV=%d TXE=%d MDM=%d) "
            "inti=%02x lsr=%02x(DR=%d TXE=%d TE=%d) txe_ack=%d rxb=%02x txb=%02x "
            "mcr=%02x(DTR=%d)\n",
            i, ap->name, ap->inte,
            !!(ap->inte & INTE_RDAV), !!(ap->inte & INTE_TXE),
            !!(ap->inte & INTE_MDM),
            ap->inti,
            ap->lsr, !!(ap->lsr & LSR_DR), !!(ap->lsr & LSR_TXE),
            !!(ap->lsr & LSR_TE), ap->txe_ack, ap->rxb, ap->txb,
            ap->mcr, !!(ap->mcr & MCR_DTR));
    }
}

/*
 * The SIGUSR2 interrupt-controller dump: the 8259's own registers, which
 * the guest's tty/dev state does not record and a wedged interrupt line
 * cannot be diagnosed without.  multio_dump() prints the same fields as a
 * debugger command; this one goes to stdout for the SIGUSR2 kdump.
 */
void
multio_pic_dump(void)
{
    struct pic *m = &pics[0];
    struct pic *s = &pics[1];

    printf("\n== interrupt controller (8259) ==\n");
    printf("  master imr=%02x (enables %02x) irr=%02x isr=%02x priority=%d\n",
        m->imr, (~m->imr) & 0xff, m->irr, m->isr, m->priority);
    printf("  slave  imr=%02x (enables %02x) irr=%02x isr=%02x priority=%d\n",
        s->imr, (~s->imr) & 0xff, s->irr, s->isr, s->priority);
    printf("  rotate=%d auto_eoi=%d ivec=%s iv_isrbit=%02x\n",
        m->rotate, m->auto_eoi, ivs_name[ivecstate], iv_isrbit);
    printf("  int_line=%d int_pin=%d vi_lines=%02x\n",
        int_line, int_pin, vi_lines);
    printf("  vector=%02x %02x %02x pic_state=%d\n",
        vector[0], vector[1], vector[2], m->pic_state);
}

/*
 * Open the terminals the -c xterm mask asks for.  uart0 is the console,
 * and the boot rom, the loader and pr() write to it from reset, before
 * anything has opened a tty, so its window must exist up front or that
 * output is lost.  uart1 and uart2 have no such firmware - nothing
 * reaches them until init opens /dev/ttyB or /dev/ttyC and a login starts
 * writing - so their xterms are opened lazily, on first traffic, from
 * wr_txb.
 *
 * This is per board.  The SS1's 2651 opens its own window from the same
 * bit; the first ACE's window is not the other board's, and neither is
 * the input behind it.
 */
static void
multio_open_xterms(int termmask)
{
    char tname[10];
    int i;

    for (i = 0; i < 3; i++) {
        if (!(termmask & (1 << i)))
            continue;
        if (i == 0) {
            sprintf(tname, "multio-%d", i);
            open_terminal(tname, 0, &ace[i].infd, &ace[i].outfd, 0, 0);
        } else {
            ace[i].want_xterm = 1;
        }
    }
}

static int
multio_init()
{
    struct termios tio;
    int termmask = 0;
    int i;
    int pollpid;
    char tname[10];

    atexit(multio_restore_terminal);

    for (i = 0; i < 3; i++) {
        ace[i].infd = -1;
    }

    for (i = 0; i < 3; i++) {
        sprintf(tname, "uart%d", i);
        ace_init(i, strdup(tname), 3 + i);
    }

    if (config_sw & CONF_SET) {
        termmask = (config_sw >> 8) & 0x7;
    }

    /*
     * The group select is the board's door: a write to it is what makes
     * the next eight ports at 0x48 an ACE's registers.  Register it in
     * both cases, so the first ACE can still be a console window when the
     * SS1 owns the interrupts.
     */
    register_output(MULTIO_MASTER+7, &multio_select);

    /*
     * When the SS1 is the interrupt controller this board is not the
     * console: its PIC, its clock and the claim on the shared input all
     * stay with the SS1, and it does not borrow the invoking terminal.
     * Its terminals are its own, though, and the mask may still ask for
     * the first ACE, so open them and leave the rest dormant.
     */
    if (!wiring_active("multio")) {
        /*
         * The monitor programs this board's PIC when it finds the
         * Wunderbus console, before the kernel's probe picks the SS1.  A
         * real 8259 takes that sequence beside the SS1's pair, so model
         * it: the line callback is the board's, and it stays quiet when
         * the board is not the wired controller.
         */
        pic_init(&pics[0]);
        pic_init(&pics[1]);
        pics[0].name = "multio";
        pics[1].name = "multio slave";
        pics[0].line_changed = multio_set_int_line;
        pics[1].line_changed = multio_set_int_line;

        multio_open_xterms(termmask);
        return 0;
    }

    // if terminal 0 is not an xterm, use mytty - this requires poll child to get SIGIO
    if (!(termmask & 0x1)) {
        int pipe_down[2];
        int pipe_up[2];
        int mypid = getpid();

        if (!myttyname) {
            fprintf(stderr, "uart 0 wants the controlling terminal, but "
                "stdin is not one.\nstart from a terminal, or put uart 0 "
                "in an xterm with -c 0x100\n");
            exit(1);
        }
        termfd = open(myttyname, O_RDWR);
        if (termfd == -1) {
            perror("terminal");
        }
        tcgetattr(termfd, &original_tio);

        pipe(pipe_down);
        pipe(pipe_up);

        pollpid = fork();
        if (!pollpid) {                     // poll child
            struct timeval timeout;
            fd_set readfds;
            int fds;
            char buf[1024];

            tcgetattr(termfd, &tio);
            cfmakeraw(&tio);
            tcsetattr(termfd, TCSANOW, &tio);
            fds = pipe_down[0];
            if (fds < termfd) fds = termfd;
            fds++;

            while (1) {
                FD_ZERO(&readfds);
                FD_SET(termfd, &readfds);            
                FD_SET(pipe_down[0], &readfds);            
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                select(fds, &readfds, 0, 0, &timeout);
                ioctl(termfd, FIONREAD, &i);
                if (i) {
                    if (i > sizeof(buf)) i = sizeof(buf);
                    i = read(termfd, buf, i);
                    write(pipe_up[1], buf, i);
                }
                ioctl(pipe_down[0], FIONREAD, &i);
                if (i) {
                    if (i > sizeof(buf)) i = sizeof(buf);
                    i = read(pipe_down[0], buf, i);
                    write(termfd, buf, i);
                }
            }
        }
        ace[0].infd = pipe_up[0];
        ace[0].outfd = pipe_down[1];
    }

    multio_open_xterms(termmask);

    terminal_fd_in = ace[0].infd;
    terminal_fd_out = ace[0].outfd;

    register_output(SBASE+7, &slave_select);

    vi_change = &multio_vi_change;
    get_intack = &multio_intack;

    /*
     * The two PICs.  Both fold into the same INT line (the slave's INT
     * reaches the master as IR6 in multio_set_int_line), and both trace
     * and name their bits the same way, so the only difference is which
     * port range reaches them.
     */
    pic_init(&pics[0]);
    pic_init(&pics[1]);
    pics[0].name = "multio";
    pics[1].name = "multio slave";
#ifndef NODEBUG
    pics[0].trace_flag = trace_multio;
    pics[1].trace_flag = trace_multio;
#endif
    pics[0].irbits = intbits;
    pics[1].irbits = intbits;
    pics[0].line_changed = multio_set_int_line;
    pics[1].line_changed = multio_set_int_line;

    /*
     * The slave MultIO (0x40) is only ever used for its PIC here - its
     * serial ports are not modelled - so its two PIC registers are wired
     * up once and left, rather than through a group-select dance like the
     * master's.  picfor() sends them to pics[1] on the address alone.
     */
    register_input(SBASE + 4, &rd_pic_port_0);
    register_input(SBASE + 5, &rd_pic_port_1);
    register_output(SBASE + 4, &wr_pic_port_0);
    register_output(SBASE + 5, &wr_pic_port_1);

    /*
     * Names for the master's eight inputs, for the state dump.  Two are
     * the S-100 bus lines this board brings in - VI0 (hd-dma) and VI1
     * (djdma) - and the cascade line IR2 carries the slave's INT.  The
     * serial ports name their own inputs from ace_init() above.
     */
    reg_intbit(0, "hd-dma");
    reg_intbit(1, "djdma");
    reg_intbit(2, "cascade");
    reg_intbit(7, "clock");
#ifndef NODEBUG
    register_mon_cmd('M', "\tdump multio state", multio_dump);
#endif

    return 0;
}

static int
multio_setup()
{
    /*
     * Only needed when uart 0 is going to borrow the terminal we were
     * started from, which is the case when bit 0 of the xterm mask is
     * clear.  ttyname answers NULL when stdin is not a terminal at all -
     * started from a script, or with stdin redirected - and strdup of
     * that is a crash before any of the machine exists.  Leave it null
     * and let the code that wants a terminal complain about it.
     */
    char *tty = ttyname(0);

    myttyname = tty ? strdup(tty) : 0;
#ifndef NODEBUG
    trace_multio = register_trace("multio");
    trace_uart = register_trace("uart");
    trace_clock = register_trace("clock");
    trace_intr = register_trace("intr");
    trace_intack = register_trace("intack");
#endif
    return 0;
}

static void
multio_usage()
{
    printf("config switch values:\n");
    printf("\t0x0700 - xterm tty enable mask\n");
    printf("\t0x1000 - no clock\n");
    printf("\t0x0800 - 1 hz clock\n");
}

struct driver multio_driver = {
    "multio",
    &multio_usage,
    &multio_setup,
    &multio_init,
    &multio_poll
};

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_multio_driver()
{
        register_driver(&multio_driver);
    }

    /*
     * vim: tabstop=4 shiftwidth=4 expandtab:
     */
