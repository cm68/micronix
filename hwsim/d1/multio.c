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

extern int_level int_s100;

int trace_multio;
int trace_uart;
int trace_noclock;

#define HZ_1    0x800

/*
 * there's one of these for every ACE.
 */
struct ace {
    int outfd;
    int infd;
    byte lcr;           // line control
    byte lsr;           // line status register
    byte mcr;           // modem control character
    byte msr;           // modem status character
    byte dll;           // baud rate lo
    byte dlm;           // baud rate hi
    byte inte;          // interrupt enables
    byte loop;          // loopback count 
    byte loopc;         // loopback character
    byte inti;          // interrupt id
    int_line vector;    // assert when something happens
} ace[3];

#define	MULTIO_PORT	0x48	// multio standard port

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
#define INTI_NOINT  0x01    // no interrupt
#define INTI_MASK   0x06    // mask of interrupt causes
#define     INTI_RSTAT  0x6     // reciever status
#define     INTI_RDAV   0x4     // rx char available
#define     INTI_TXE    0x2     // tx buffer empty
#define     INTI_MDM    0x0     // modem status change

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

static byte group;
static struct ace *acep;

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
        sprintf(sbuf, "%x %d", v, v);
        break; 
    }
    return sbuf;
}

static char *intbits[] = { "", "", "", "", "", "", "", "" };
static char *icw1_bits[] = { "icw4need", "single", "interval4", "level", "icw1", 0, 0, 0 };
static char *ocw2_bits[] = { 0, 0, 0, 0, 0, "eoi", "spec", "rotate" };
static char *ocw3_bits[] = { "ris", "rr", "poll", "ocw3", 0, "smm", "esmm", 0, 0 };
static char *icw4_bits[] = { "8086", "autoeoi", "master", "buffered", "special", 0, 0, 0 };
static char *nobits[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

#define IRQ0    0x01        // vi0 - hddma/hdca
#define IRQ1    0x02        // vi1 - djdma
#define IRQ2    0x04        // vi2
#define IRQ3    0x08        // uart 1
#define IRQ4    0x10        // uart 2
#define IRQ5    0x20        // uart 3
#define IRQ6    0x40        // daisy
#define IRQ7    0x80        // clock interrupt

/*
 * we've got an 8259, which has 2 registers at MULTIO_PORT+4 and MULTIO_PORT+5
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

byte imr;           // interrupt mask register
byte irr;           // interrupt request register
byte isr;           // in-service register

byte irq;           // actual interrupt line on s100

int pic_state = 0;
#define PS_UNDEF    0
#define PS_ICW2     1
#define PS_ICW3     2
#define PS_ICW4     3
#define PS_READY    4

int
multio_dump(char **p)
{
    printf("multio dump: group %d\n", group);
    printf("pic_state: %d\n", pic_state);
    printf("icw1: %x %s\n", icw1, bitdef(icw1, icw1_bits));
    printf("ocw2: %x %s\n", ocw2, bitdef(ocw2, ocw2_bits));
    printf("ocw3: %x %s\n", ocw3, bitdef(ocw3, ocw3_bits));
    printf("icw4: %x %s\n", icw4, bitdef(icw4, icw1_bits));
    printf("imr: %x %s\n", imr, bitdef(imr, intbits));
    printf("irr: %x %s\n", irr, bitdef(irr, intbits));
    printf("isr: %x %s\n", isr, bitdef(isr, intbits));
    printf("irq: %x int_s100: %d int_pin: %d\n", irq, int_s100, int_pin);
    return 0;
}

// return the highest bit set
static byte 
highest(byte b)
{
    byte mask = 0x80;

    while (mask) {
        if (mask & b) break;
        mask >>= 1;
    }
    return mask;
}

static void
wr_pic_port_0(portaddr p, byte v)
{
    char **bdec;
    byte *rname;
    byte high;

    if (v & PIC0_ICW1) {            // ICW1
        bdec = icw1_bits;
        rname = "icw1";
        pic_state = PS_ICW2;
        icw1 = v;
    } else if (v & PIC0_OCW3) {     // OCW3
        bdec = ocw3_bits;
        rname = "ocw3";
        ocw3 = v;
    } else {                        // OCW2
        bdec = ocw2_bits;
        rname = "ocw2";
        ocw2 = v;
        switch (ocw2 & OCW2_CMD) {
        case OCW2_NSEOI:
        case OCW2_NSEOIR:
            isr &= ~highest(isr);
            break;
        }
    }

    if (trace & trace_multio) {
        printf("multio: write pic0 %x %s: %s\n", v, rname, bitdef(v, bdec));
    }
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
        bdec = intbits;
        break;
    default:
        reg = "unknown";
        bdec = nobits;
    }
    if (trace & trace_multio) printf("multio: write pic1 %s %x, %s\n", reg, v, bitdef(v, bdec));
}

static byte 
rd_pic_port_0(portaddr p)
{
    if (trace & trace_multio) printf("multio: read pic0\n");
    return 0;
}

static byte 
rd_pic_port_1(portaddr p)
{
    if (trace & trace_multio) printf("multio: read pic1\n");
    return 0;
}

// set the level of the interrupt line
void
vi_handler(int_line signal, int_level level)
{
    byte mask;

    if (trace & trace_multio) printf("multio: vi_handler %d %s %s\n", 
        signal, intbits[signal], (level == int_set) ? "set" : "clear");

    switch(signal) {
    case vi_0:  mask = IRQ0; break;
    case vi_1:  mask = IRQ1; break;
    case vi_2:  mask = IRQ2; break;
    case vi_3:  mask = IRQ3; break;
    case vi_4:  mask = IRQ4; break;
    case vi_5:  mask = IRQ5; break;
    case vi_6:  mask = IRQ6; break;
    case vi_7:  mask = IRQ7; break;
    default:
        printf("vi_handler unhandled signal %d %s\n", 
            signal, (level == int_set) ? "set" : "clear");
        return;
    }
    if (level == int_set) {
        irr |= mask;
    } else {
        irr &= ~mask;
    }

    // if any unmasked interrupts are high, assert int
    if (irr & ~imr) {
        irq = 1;
        set_interrupt(interrupt, int_set);
    } else {
        irq = 0;
        set_interrupt(interrupt, int_clear);
    }
}


// run an interrupt ack cycle and return the vector byte(s) to the cpu card
intvec
multio_intvec()
{
    byte mask = 0x01;
    int i = 0;
    intvec iv;

    while (mask) {
        if ((mask & irr) && !(mask & imr)) {
            break;
        }
        mask <<= 1;
        i++;
    }
    if (!mask) {
        printf("lose: multio_intvec no interrupt irr %x imr %x\n", irr, imr);
        multio_dump(0);
        return 0;
    }

    isr |= mask; 
    iv = INTA_LEN | (VECTOR(i) << 8) | 0xcd;
    if (trace & trace_multio) printf("multio: multio_intvec %08x\n", iv);
    return (iv);
}

/*
 * set the state of the interrupt line appropriately conditioned on enable bits
 * these are prioritized
 * we only implement TXE and RDAV.
 */
void
uart_interrupt_check(struct ace *ap)
{
    int bytes;

    ap->inti = INTI_NOINT;
    if (ap->inte & INTE_RDAV) {
        ioctl(ap->infd, FIONREAD, &bytes);
        if (bytes) {
            ap->inti = INTI_RDAV;
        }
    } else if (ap->inte & INTE_TXE) {
        ap->inti = INTI_TXE;
    }
    if (ap->inti == INTI_NOINT) {
        set_interrupt(ap->vector, int_clear);
    } else {
        set_interrupt(ap->vector, int_set);
    }
}

/*
 * recieve buffer.  
 * the selftest uses the loopback function, so we need to handle it.
 * while loopback is set in the mcr, then we return any character written to txb.
 * if LCR_DLAB is set, access to lsb of baud rate divisor
 */
static byte 
rd_rxb(portaddr p) 
{
    byte retval;
    int bytes;

    if (acep->lcr & LCR_DLAB) {
        return acep->dll;
    }
    if (acep->mcr & MCR_LOOP) {
        if (!acep->loop) {
            if (trace & trace_uart) printf("uart: read unwritten loopback\n");
        }
        acep->loop = 0;
        retval = acep->loopc;
    } else if (acep->infd == -1) {
        retval = 0;
    } else {
        ioctl(acep->infd, FIONREAD, &bytes);
        if (bytes) {
            if (read(acep->infd, &retval, 1) != 1) {
                if (trace & trace_uart) printf("uart: rd_rxb failed\n");
            }
        } else {
            retval = 0;
        }
    }
    // if we set a read interrupt, maybe clear it
    uart_interrupt_check(acep);

    if (trace & trace_uart) {
        printf("uart: read rxb = %s%s\n",
            acep->mcr & MCR_LOOP ? "(loopback) " : "", printable(retval));
    }
    return retval;
}

/*
 * transmitter buffer
 * if LCR_DLAB is set, access to lsb of baud rate divisor
 */
static void
wr_txb(portaddr p, byte v) 
{
    if (acep->lcr & LCR_DLAB) {
        acep->dll = v;
        return;
    }
    if (acep->mcr & MCR_LOOP) {
        acep->loopc = v;
        acep->loop = 1;
    } else if (acep->outfd != -1) {
        write(acep->outfd, &v, 1);
    }
    if (trace & trace_uart) {
        printf("uart: write txb %s%s\n", 
            acep->mcr & MCR_LOOP ? "(loopback) " : "", printable(v));
    }
}

/*
 * interrupt enable register
 * if LCR_DLAB is set, access to msb of baud rate divisor
 */
static char *inte_bits[] = { "READAVAIL", "TXHOLDEMPTY", "RLINESTAT", "MDMSTAT", 0, 0, 0, 0 };
static byte 
rd_inte(portaddr p)
{
    if (acep->lcr & LCR_DLAB) {
        return acep->dlm;
    }
    if (trace & trace_uart) {
        printf("uart: read inte %x %s\n", acep->inte, bitdef(acep->inte, inte_bits));
    }
    return acep->inte;
}

static void
wr_inte(portaddr p, byte v)
{
    if (acep->lcr & LCR_DLAB) {
        acep->dlm = v;
        return;
    }
    acep->inte = v;
    if (trace & trace_uart) {
        printf("uart: write inte %x %s\n", v, bitdef(v, inte_bits));
    }
}

/*
 * line control register
 * contains the divisor access bit that changes the meaning of txb,rxb,and inte
 */
static char *lcr_bits[] = { "WLS0", "WLS1", "STB", "PEN", "EPS", "STP", "SBRK", "DLAB" };
static byte 
rd_lcr(portaddr p)
{
    if (trace & trace_uart) {
        printf("uart: read lcr %x %s\n", acep->lcr, bitdef(acep->lcr, lcr_bits));
    }
    return acep->lcr;
}

static void
wr_lcr(portaddr p, byte v)
{
    acep->lcr = v;
    if (trace & trace_uart) {
        printf("uart: write lcr %x %s\n", acep->lcr, bitdef(acep->lcr, lcr_bits));
    }
}

/*
 * line status register.
 */
char *lsr_bits[] = { "DR" , "OVER", "PERR", "FERR", "BRK", "TXE", "TE", 0 };
static byte 
rd_lsr(portaddr p)
{
    int bytes = 0;

    if (acep->mcr & MCR_LOOP) {
        bytes = acep->loop;
    } else if (acep->infd != -1) {
        ioctl(acep->infd, FIONREAD, &bytes);
    }
    acep->lsr = 0;
    acep->lsr |= LSR_TXE | (bytes ? LSR_DR : 0);

    if (trace & trace_uart) {
        printf("uart: read linestat %x %s\n", acep->lsr, bitdef(acep->lsr, lsr_bits));
    }
    return acep->lsr;
}

// the chip docs say this register is not well defined for write.
static void
wr_lsr(portaddr p, byte v)
{
    if (trace & trace_uart) printf("uart: write linestat %x %s\n", v, bitdef(v, lsr_bits));
}

/*
 * modem control register.
 * contains the loopback enable
 */
static char *mcr_bits[] = { "DTR", "RTS", "OUT1", "OUT2", "LOOP", 0, 0, 0 };
static byte 
rd_mcr(portaddr p)
{
    if (trace & trace_uart) {
        printf("uart: read mdmctl %x %s\n", acep->mcr, bitdef(acep->mcr, mcr_bits));
    }
    return acep->mcr;
}

static void
wr_mcr(portaddr p, byte v)
{
    // if a new set of loopback
    if ((v & MCR_LOOP) && !(acep->mcr & MCR_LOOP)) {
        acep->loop = 0;
    }
    acep->mcr = v;
    if (trace & trace_uart) {
        printf("uart: write mdmctl %x %s\n", acep->mcr, bitdef(acep->mcr, mcr_bits));
    }
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
    if (trace & trace_uart) {
        printf("uart: read inti\n");
    }
    return 0;
}

static char *msr_bits[] = { "DCTS", "DDSR", "TERI", "DDCD", "CTS", "DSR", "RI", "DCD" };
static byte 
rd_msr(portaddr p)
{
    if (trace & trace_uart) {
        printf("uart: read mdmstat %x %s\n", acep->msr, bitdef(acep->msr, msr_bits));
    }
    return 0;
}

// it is unclear from the chip doc what happens when you write this register
static void 
wr_msr(portaddr p, byte v)
{
    acep->msr = v;

    if (trace & trace_uart) {
        printf("uart: write mdmstat %x %s\n", acep->msr, bitdef(acep->msr, msr_bits));
    }
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

void
clock_handler()
{
    set_interrupt(vi_7, int_set);
}

/*
 * the real time clock is latched this 40 bit structure
 */
byte rtc[5] = { 0x25, 0x34, 0x12, 0x25, 0x76 };
int rtcptr;
static byte last_wrclock;
time_t now;

char *wclk_bits[] = { "DATA", "DSTROBE", 0, 0, 0, "CSTROBE", 0, 0 };
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

    if (trace & trace_multio) {
        printf("multio: write clock %x %s %s\n", 
            v, clk_cmd[(v & CLK_CMD) >> 2], bitdef(v, wclk_bits));
    }

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
            now = time(&now);
            tm = localtime(&now);
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
            if (!(trace & trace_noclock)) {
                recurring_timeout("multio_clock", rate, clock_handler);
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

    set_interrupt(vi_7, int_clear);

    v = (rtc[rtcptr / 8] >> (rtcptr % 8)) & 1;
    if (trace & trace_multio) printf("multio: read clock %x\n", v);
    return v;
}

static byte
rd_daisy(portaddr p)
{
    byte ret;

    ret = 0;
    if (trace & trace_multio) printf("multio: read daisy %x = 0x%x\n", p, ret);
    return ret;
}

static void
wr_daisy0(portaddr p, byte v)
{
    if (trace & trace_multio) printf("multio: write daisy0 %x\n", v);
}

static void
wr_daisy1(portaddr p, byte v)
{
    if (trace & trace_multio) printf("multio: write daisy1 %x\n", v);
}

static byte
undef_inreg(portaddr p)
{
    if (trace & trace_multio) printf("multio: read to %x+%x undefined\n", MULTIO_PORT, p - MULTIO_PORT);
    return 0;
}

static void
undef_outreg(portaddr p, byte v)
{
    if (trace & trace_multio) printf("multio: write 0x%x to undefined %x+%x\n", v, MULTIO_PORT, p - MULTIO_PORT);
}

static char *group_bits[] = { 0, 0, "membank", "intena", "printreset", "parstb", 0, 0 };

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

    if (trace & trace_multio) {
        printf("multio: write group select %x %s\n", group, bitdef(v, group_bits));
    }

    switch (group) {
    case GRP0:  // parallel port, clock, pic
        register_input(MULTIO_PORT + 0, &rd_daisy);
        register_input(MULTIO_PORT + 1, &undef_inreg);
        register_input(MULTIO_PORT + 2, &rd_clock);
        register_input(MULTIO_PORT + 3, &undef_inreg);
        register_input(MULTIO_PORT + 4, &rd_pic_port_0);
        register_input(MULTIO_PORT + 5, &rd_pic_port_1);
        register_input(MULTIO_PORT + 6, &undef_inreg);

        register_output(MULTIO_PORT + 0, &wr_daisy0);
        register_output(MULTIO_PORT + 1, &wr_daisy1);
        register_output(MULTIO_PORT + 2, &wr_clock);
        register_output(MULTIO_PORT + 3, &undef_outreg);
        register_output(MULTIO_PORT + 4, &wr_pic_port_0);
        register_output(MULTIO_PORT + 5, &wr_pic_port_1);
        register_output(MULTIO_PORT + 6, &undef_outreg);
        break; 
    case GRP1:  // serial ports
    case GRP2:
    case GRP3:
        acep = &ace[group - 1];
        register_input(MULTIO_PORT + 0, &rd_rxb);
        register_input(MULTIO_PORT + 1, &rd_inte);
        register_input(MULTIO_PORT + 2, &rd_inti);
        register_input(MULTIO_PORT + 3, &rd_lcr);
        register_input(MULTIO_PORT + 4, &rd_mcr);
        register_input(MULTIO_PORT + 5, &rd_lsr);
        register_input(MULTIO_PORT + 6, &rd_msr);

        register_output(MULTIO_PORT + 0, &wr_txb);
        register_output(MULTIO_PORT + 1, &wr_inte);
        register_output(MULTIO_PORT + 2, &undef_outreg);
        register_output(MULTIO_PORT + 3, &wr_lcr);
        register_output(MULTIO_PORT + 4, &wr_mcr);
        register_output(MULTIO_PORT + 5, &wr_lsr);
        register_output(MULTIO_PORT + 6, &wr_msr);
        break;
    }
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

void
reg_intbit(int_line signal, char *name)
{
    intbits[signal] = name;
}

void
sigio_handler(int sig)
{
    int i;
 
    for (i = 0; i < 3; i++) {
        if (ace[i].infd == -1) {
            continue;
        }
        uart_interrupt_check(&ace[i]);
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

    atexit(exit_hook);

    for (i = 0; i < 3; i++) {
        ace[i].infd = -1;
    }

    if (config_sw & CONF_SET) {
        termmask = (config_sw >> 8) & 0x7;
    }

    mysignal(SIGIO, sigio_handler);

    for (i = 0; i < 3; i++) {
        ace[i].infd = ace[i].outfd = -1;
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
                    kill(mypid, SIGIO);
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
            open_terminal(tname, SIGIO, &ace[i].infd, &ace[i].outfd, 0, 0);
        }
    }

    terminal_fd_in = ace[0].infd;
    terminal_fd_out = ace[0].outfd;

	register_output(MULTIO_PORT+7, &multio_select);
    reg_intbit(vi_0, "hd");
    reg_intbit(vi_1, "djdma");
    reg_intbit(vi_7, "clock");
    register_interrupt(vi_0, vi_handler);               // hdca and hddma
    register_interrupt(vi_1, vi_handler);               // djdma
    if (ace[0].infd != -1) {
        reg_intbit(vi_3, "uart1");
        register_interrupt(vi_3, vi_handler);
        ace[0].vector = vi_3;
    }
    if (ace[1].infd != -1) {
        reg_intbit(vi_4, "uart2");
        register_interrupt(vi_4, vi_handler);
        ace[1].vector = vi_4;
    }
    if (ace[2].infd != -1) {
        reg_intbit(vi_5, "uart3");
        register_interrupt(vi_5, vi_handler);
        ace[2].vector = vi_4;
    }
    register_interrupt(vi_7, vi_handler);               // clock

    register_intvec(multio_intvec);
    register_mon_cmd('M', "\tdump multio state", multio_dump);

    return 0;
}

void
multio_usage()
{
    printf("config switch values:\n");
    printf("\t0x0700 - xterm tty enable mask\n");
    printf("\t0x0800 - 1 hz clock\n");
}

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_multio_driver()
{
    myttyname = strdup(ttyname(0));
    trace_multio = register_trace("multio");
    trace_uart = register_trace("uart");
    trace_noclock = register_trace("noclock");
    register_usage_hook(multio_usage);
    register_startup_hook(multio_init);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
