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
#include "util.h"
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

int trace_multio;
int trace_uart;
int trace_noclock;
int trace_intr;
int trace_intack;

#define HZ_1    0x800

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

    int line;
    u64 chartime;       // shift time through the uart
} ace[3];

#define TXE_TIMEOUT     5000     // how long to wait before setting LSR_TXE

#define	MULTIO_MASTER	0x48	// multio master standard port
#define	MULTIO_SLAVE	0x58	// multio slave standard port

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
static char *icw1_bits[] = { "icw4need", "single", "interval4", "level", "icw1", 0, 0, 0 };
static char *ocw2_bits[] = { 0, 0, 0, 0, 0, "eoi", "spec", "rotate" };
static char *ocw3_bits[] = { "ris", "rr", "poll", "ocw3", 0, "smm", "esmm", 0, 0 };
static char *icw4_bits[] = { "8086", "autoeoi", "master", "buffered", "special", 0, 0, 0 };
static char *nobits[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
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
 * we've got an 8259, which has 2 registers at MULTIO_MASTER+4 and MULTIO_MASTER+5
 */
#define PIC0_ICW1   0x10
#define PIC0_OCW3   0x08

byte icw1;
#define ICW1_I4     0x01        // icw4 needed
#define ICW1_SNG    0x02        // single, no cascade
#define ICW1_ADI    0x04        // address interval = 4
#define ICW1_LTIM   0x08        // level triggered mode
#define ICW1_VECL   0xe0        // bits 7-5 of interrupt vector

byte icw2;
byte icw3;
byte icw4;

#define VECTOR(i)    ((icw2 << 8) + (icw1 & ICW1_VECL) + ((i) * ((icw1 & ICW1_ADI) ? 4 : 8)))

byte ocw2;
#define OCW2_LEVEL  0x07        // level mask
#define OCW2_CMD    0xe0        // command mask
#define OCW2_NSEOI  0x20        // non-specific eoi
#define OCW2_NSEOIR 0xa0        // non-specific eoi with rotate

byte ocw3;
#define OCW3_RIS    0x01        // register to read
#define OCW3_RR     0x02        // register to read
#define OCW3_POLL   0x04        // poll command
#define OCW3_SMM    0x20        // set special mask mode
#define OCW3_ESMM   0x40        // affect special mask mode

byte imr;           // interrupt mask register (enables are low)
byte irr;           // interrupt request register
byte isr;           // in-service register


int priority;       // interrupt with current highest priority

int pic_state = 0;
#define PS_UNDEF    0
#define PS_ICW2     1
#define PS_ICW3     2
#define PS_ICW4     3
#define PS_READY    4

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
    l("multio dump: group %d\n", group);
    l("\tpic_state: %d\n", pic_state);
    l("\ticw1: %02x %s\n", icw1, bitdef(icw1, icw1_bits));
    l("\tocw2: %02x %s\n", ocw2, bitdef(ocw2, ocw2_bits));
    l("\tocw3: %02x %s\n", ocw3, bitdef(ocw3, ocw3_bits));
    l("\ticw4: %02x %s\n", icw4, bitdef(icw4, icw4_bits));
    l("\timr: %02x %s\n", (~imr) & 0xff, bitdef(~imr, intbits));
    l("\tirr: %02x %s\n", irr, bitdef(irr, intbits));
    l("\tisr: %02x %s\n", isr, bitdef(isr, intbits));
    l("\tvecbase: %04x\n", (icw1 & ICW1_VECL) + (icw2 << 8));
    l("\tpriority: %d %02x\n", priority, 1 << priority);
    l("\tint_line: %d int_pin: %d vi_lines %02x\n", int_line, int_pin, vi_lines);
    l("\tivecstate: %s isrbit: %02x vector %02x %02x %02x\n",
        ivs_name[ivecstate], iv_isrbit, vector[0], vector[1], vector[2]);
    return 0;
}

int
bitnum(byte m)
{
    int i;
    int b;

    for (i = 0; i < 8; i++) {
        b = (priority + i) % 8;
        if (m & (1 << b)) {
            if (m != (1 << b)) {
                l("bitnum multiple bits %02x\n", m);
                multio_dump(0);
            }
            return b;
        }
    }
    l("bitnum for no bits set\n");
    multio_dump(0);
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

static void
wr_pic_port_0(portaddr p, byte v)
{
    char **bdec;
    byte *rname;
    int intlevel;

    if (v & PIC0_ICW1) {            // ICW1
        bdec = icw1_bits;
        rname = "icw1";
        pic_state = PS_ICW2;
        icw1 = v;
        priority = 0;
        if (!(icw1 & ICW1_LTIM)) {
            l("multio:  edge triggered not supported\n");
        } 
    } else if (v & PIC0_OCW3) {     // OCW3
        bdec = ocw3_bits;
        rname = "ocw3";
        ocw3 = v;
    } else {                        // OCW2
        bdec = ocw2_bits;
        rname = "ocw2";
        ocw2 = v;
        switch (ocw2 & OCW2_CMD) {
        case OCW2_NSEOIR:
            intlevel = bitnum(isr);
            trace(trace_multio, "multio: NSEOIR isr:%02x lvl:%d pri:%d\n", 
                isr, intlevel, priority);
            priority = (intlevel + 1) % 8;
            isr ^= (1 << intlevel);
            break;
        case OCW2_NSEOI:
            isr = 0;
            break;
        default:
            l("pic bogus ocw2 write command %x\n", ocw2);
        }
    }

    trace(trace_multio, "multio: write pic0 %02x %s: %s %x\n", 
        v, rname, bitdef(v, bdec), isr);
}

static void
wr_pic_port_1(portaddr p, byte v)
{
    char **bdec;
    byte *reg;

    switch (pic_state) {
    case PS_ICW2:
        reg = "icw2";
        icw2 = v;
        bdec = nobits;
        if (!(icw1 & ICW1_SNG)) {
            pic_state = PS_ICW3;
        } else if (icw1 & ICW1_I4) {
            pic_state = PS_ICW4;
        }
        break;
    case PS_ICW3:
        reg = "icw3";
        icw3 = v;
        bdec = nobits;
        if (icw1 & ICW1_I4) {
            pic_state = PS_ICW4;
        } else {
            pic_state = PS_READY;
        }
        break;
    case PS_ICW4:
        reg = "icw4";
        icw4 = v;
        bdec = icw4_bits;
        pic_state = PS_READY;
        break;
    case PS_READY:
        reg = "imr";
        imr = v;
        v = (~imr) & 0xff;
        bdec = intbits;
        break;
    default:
        reg = "unknown";
        bdec = nobits;
    }
    trace(trace_multio, "multio: write pic1 %s %02x, %s\n", 
        reg, v, bitdef(v, bdec));
}

static byte 
rd_pic_port_0(portaddr p)
{
    trace(trace_multio, "multio: read pic0\n");
    return 0;
}

static byte 
rd_pic_port_1(portaddr p)
{
    trace(trace_multio, "multio: read pic1\n");
    return 0;
}

/*
 * based on the irr, imr and isr, set the s100 int line
 */
void
multio_set_int_line()
{
    int line;

    // if any unmasked, unserviced interrupts are high, assert int
    if ((irr & ~imr) & ~isr) {
        line = 1;
    } else {
        line = 0;
    }
    trace(trace_multio, "multio: %s imr:%x irr:%x isr:%x\n", 
        line ? "set":"clear", imr, irr, isr);
    (*int_change)(line);
}

/*
 * set the level of the interrupt line
 * this set irr in the interrupt controller and possibly sets the s100 
 * interrupt line the s100 interrupt line is reset by the interrupt 
 * vector generation.
 */
void
multio_vi_change(unsigned char new)
{
    byte mask = vi_lines ^ new;

    trace(trace_multio, "multio: vi_changed lines:%x new:%x diff:%x\n", 
            vi_lines, new, mask);

    if (mask == 0) {
        l("multio: vi_change no change new:%02x prev:%02x\n", new, vi_lines);
        return;
    }
    vi_lines = new;

    // if raising interrupt
    if (new & mask) {
        irr |= mask;
    } else {
        irr &= ~mask;
    }
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
    unsigned char i;

    trace(trace_intack, "multio_intack: ivecstate=%d %s\n", 
        ivecstate, ivs_name[ivecstate]);

    // the vector is empty, so let's calculate it
    if (ivecstate == IV_EMPTY) {
        /*
         * find the interrupt request line, starting from the current highest
         * priority
         */
        for (i = 0; i < 8; i++) {
            level = (priority + i) % 8;
            mask = 1 << level;

            // if we have an unmasked request, we found our request
            if ((mask & irr) && !(mask & imr)) {
                break;
            }
        }

        // we didn't find a cause for our interrupt
        if (i == 8) {
            l("lose: no unmasked request found!\n");
            multio_dump(0);
            ivecstate = IV_INVALID;
            iv_isrbit = 0;
        } else {
            iv_isrbit = mask;
            vecaddr = (icw1 & ICW1_VECL) + (level * ((icw1 & ICW1_ADI) ? 4 : 8)) + (icw2 << 8);
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
        isr |= iv_isrbit;
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
    } else if ((ap->inte & INTE_TXE) && (ap->lsr & LSR_TXE)) {
        ap->inti = INTI_TXE;
    } else {
        ap->inti = INTI_NOINT;
    }

    // only call set_vi on a change
    if ((before == INTI_NOINT) && (ap->inti != INTI_NOINT)) {
        set_vi(ap->vi_line, 0, 1);
    } else if ((before != INTI_NOINT) && (ap->inti == INTI_NOINT)) {
        set_vi(ap->vi_line, 0, 0);
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

    t = now64();

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
    ap->rxwait = now64();
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
        ap->rxwait = now64() + ap->chartime;
        multio_set_inti(ap);
    } else {
        retval = 0;
    }
    trace(trace_uart, "%s: read rxb = %s%s\n",
         ap->name, ap->mcr & MCR_LOOP ? "(loopback) " : "", printable(retval));
    return retval;
}

/*
 * transmitter buffer
 * if LCR_DLAB is set, access to lsb of baud rate divisor
 */
static void
wr_txb(portaddr p, byte v) 
{
    struct ace *ap = select_ace();

    if (ap->lcr & LCR_DLAB) {
        trace(trace_uart, "%s: write dll %02x\n", ap->name, v);
        ap->dll = v;
        return;
    }

    // detect overrun
    if (!(ap->lsr & LSR_TXE)) {
        l("multio: send overrun on line %d\n", ap->line);
    }

    ap->txpend = now64() + ap->chartime;
    ap->txb = v;
    ap->lsr &= ~LSR_TXE;

    if (((ap->mcr & MCR_LOOP) == 0) && (ap->outfd != -1)) {
        write(ap->outfd, &v, 1);
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

    trace(trace_uart, "%s: read inti %02x %s\n", 
        ap->name, ap->inti, bitdef(ap->inti, inti_bits));

    /* XXX - this is probably busted */
    if (ap->inti == INTI_TXE) {
        ap->inti = INTI_NOINT;
        set_vi(ap->vi_line, 0, 0);
    }
    return ap->inti;
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
            break;
        case CC_SET:        // set time - no, we not going to do that
            break; 
        case CC_GET:        // read the unix time and populate the array
            nowtime = time(0);
            tm = localtime(&nowtime);
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
        if (rate != 0) {
            if (!(traceflags & trace_noclock)) {
                recurring_time_out("multio_clock", rate, clock_handler, 0);
            }
        }
    }

    // if falling edge on DSTROBE and command the same, advance the data bit pointer
    if ((!(v & CLK_SHIFT)) && (last_wrclock & CLK_SHIFT) &&
        ((v & CLK_CMD) == (last_wrclock & CLK_CMD))) {
        if (++rtcptr == 40) {
            rtcptr = 0;
        }
    }
    last_wrclock = v;
}

static byte 
rd_clock(portaddr p)
{
    byte v;

    v = (rtc[rtcptr / 8] >> (rtcptr % 8)) & 1;
    trace(trace_multio, "multio: read clock %02x\n", v);

    set_vi(7, 0, 0);

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

byte
rd_fake_daisy(portaddr p)
{
    return 0xff;
}

/*
 * this is for the second multio in a system, not present in ours
 */
void
multio_slave_select(portaddr p, byte v)
{
    register_input(MULTIO_SLAVE + 0, &rd_fake_daisy);
    return;
}

int terminal_fd_in;
int terminal_fd_out;

int termfd;
char *myttyname;

static struct termios original_tio;

static void
exit_hook()
{
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
        l(" ---- clock --- \n\n");
        set_vi(7, 0, 1);
    }
    return 0;
}

static int
multio_init()
{
    struct termios tio;
    int termmask = 0;
    int i;
    int pollpid;
    char tname[10];

    atexit(exit_hook);

    for (i = 0; i < 3; i++) {
        ace[i].infd = -1;
    }

    if (config_sw & CONF_SET) {
        termmask = (config_sw >> 8) & 0x7;
    }

    for (i = 0; i < 3; i++) {
        sprintf(tname, "uart%d", i);
        ace_init(i, strdup(tname), 3 + i);
    }

    // if terminal 0 is not an xterm, use mytty - this requires poll child to get SIGIO
    if (!(termmask & 0x1)) {
        int pipe_down[2];
        int pipe_up[2];
        int mypid = getpid();

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

    // create xterms
    for (i = 0; i < 3; i++) {
        if (termmask & (1 << i)) {
            sprintf(tname, "multio-%d", i);
            open_terminal(tname, 0, &ace[i].infd, &ace[i].outfd, 0, 0);
        }
    }

    terminal_fd_in = ace[0].infd;
    terminal_fd_out = ace[0].outfd;

	register_output(MULTIO_MASTER+7, &multio_select);
	register_output(MULTIO_SLAVE+7, &multio_slave_select);

    vi_change = &multio_vi_change;
    get_intack = &multio_intack;

    reg_intbit(0, "hd");
    reg_intbit(1, "djdma");
    reg_intbit(2, "slave");
    reg_intbit(7, "clock");
    register_mon_cmd('M', "\tdump multio state", multio_dump);

    return 0;
}

static int
multio_setup()
{
    myttyname = strdup(ttyname(0));
    trace_multio = register_trace("multio");
    trace_uart = register_trace("uart");
    trace_noclock = register_trace("noclock");
    trace_intr = register_trace("intr");
    trace_intack = register_trace("intack");
    return 0;
}

static void
multio_usage()
{
    printf("config switch values:\n");
    printf("\t0x0700 - xterm tty enable mask\n");
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
