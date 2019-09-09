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

int trace_multio;
int trace_uart;

struct terminal {
    int outfd;
    int infd;
} terminals[3];

#define	MULTIO_PORT	0x48	// multio standard port

/*
 * the multio select port is used to enable one of 4 groups and other functions
 */
#define GROUP_MASK  0x03
#define MEM_BANK    0x04
#define INTENA      0x08
#define PREST       0x10
#define PAROUT      0x20

#define GRP0        0x00    // pports, clock, pic
#define GRP1        0x01    // serial 1
#define GRP2        0x02    // serial 2
#define GRP3        0x03    // serial 3

// linestat
#define LSR_DR      0x01    // input data ready
#define LSR_OVER    0x02    // input data overrun
#define LSR_PERR    0x04    // input parity error
#define LSR_FERR    0x08    // input framing error
#define LSR_BRK     0x10    // break
#define LSR_TBE     0x20    // transmit buffer empty
#define LSR_TE      0x40    // transmitter empty

// modem control register
#define MCR_DTR     0x01    // data terminal ready
#define MCR_RTS     0x02    // request to send
#define MCR_LOOP    0x10    // loopback

// line control register
#define LCR_82      0x07    // 8 data bits, 2 stop bits
#define LCR_DLAB    0x80    // accessio baud in inte, txb

// interrupt enable register
#define INTE_RDAV   0x01    // read available
#define INTE_TXE    0x02    // tx hold empty
#define INTE_RLINE  0x04    // read line status change
#define INTE_MDM    0x08    // modem status change

static byte group;

/* read/write baud rate */
static byte dlab[3];
static byte dll[3];
static byte dlm[3];
static byte inte[3];
static byte loop[3];        // has a character been written to loopback
static byte loopc[3];       // if so, what is it
static byte mcr[3];
static byte lcr[3];

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

int pic_state = 0;
#define PS_UNDEF    0
#define PS_ICW2     1
#define PS_ICW3     2
#define PS_ICW4     3
#define PS_READY    4

// return the highest bit set
static byte 
highest(byte b)
{
    byte mask;
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
        set_interrupt(interrupt, int_set);
    } else {
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
        return 0;
    }

    isr |= mask; 
    iv = INTA_LEN | (VECTOR(i) << 8) | 0xcd;
    if (trace & trace_multio) printf("multio: multio_intvec %08x\n", iv);
    return (iv);
}

static byte 
rd_rxb(portaddr p)
{
    byte retval;
    int bytes;

    if (dlab[group - 1]) {
        return dll[group - 1];
    }
    if (mcr[group - 1] & MCR_LOOP) {
        if (!loop[group - 1]) {
            if (trace & trace_uart) printf("uart: read unwritten loopback\n");
        }
        loop[group - 1] = 0;
        retval = loopc[group - 1];
    } else {
        ioctl(terminals[group - 1].infd, FIONREAD, &bytes);
        if (bytes) {
            if (read(terminals[group - 1].infd, &retval, 1) != 1) {
                if (trace & trace_uart) printf("uart: rd_rxb failed\n");
            }
        } else {
            retval = 0;
        }
    }
    if (trace & trace_uart) printf("uart: read rxb = %s\n", printable(retval));
    return retval;
}

static byte 
rd_inte(portaddr p)
{
    if (dlab[group - 1]) {
        return dlm[group - 1];
    }
    if (trace & trace_uart) printf("uart: read inte\n");
    return 0;
}

static byte 
rd_inti(portaddr p)
{
    if (trace & trace_uart) printf("uart: read inti\n");
    return 0;
}

static byte 
rd_linectl(portaddr p)
{
    if (trace & trace_uart) printf("uart: read linectl\n");
    return 0;
}

static byte 
rd_mdmctl(portaddr p)
{
    if (trace & trace_uart) printf("uart: read mdmctl\n");
    return 0;
}

char *ls_bits[] = { "DR" , "OVER", "PERR", "FERR", "BRK", "TBE", "TE", 0 };
static byte 
rd_linestat(portaddr p)
{
    int bytes = 0;
    byte retval;

    if (mcr[group - 1] & MCR_LOOP) {
        bytes = loop[group - 1];
    } else {
        ioctl(terminals[group - 1].infd, FIONREAD, &bytes);
    }

    retval = LSR_TBE | (bytes ? LSR_DR : 0);
    if (trace & trace_uart) printf("uart: read linestat %x %s\n", retval, bitdef(retval, ls_bits));
    return retval;
}

static byte 
rd_mdmstat(portaddr p)
{
    if (trace & trace_uart) printf("uart: read mdmstat\n");
    return 0;
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
#define     CC_32HZ     0x1c
#define CLK_SETCMD  0x20    // strobe the command

#define  bcd(h,l)    ((((h) & 0xf) << 4) | ((l) & 0xf))

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
        case CC_32HZ:
            rate = 32;
            break;
        }
        if (rate != 0) {
            struct itimerval itimer;
            itimer.it_interval.tv_sec = 0;
            itimer.it_interval.tv_usec = 1000000/rate;
            setitimer(ITIMER_REAL, &itimer, 0);
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
    if (trace & trace_multio) printf("multio: read clock %x\n", v);
    return v;
}

static void
wr_txb(portaddr p, byte v)
{
    if (dlab[group - 1]) {
        dll[group - 1] = v;
        return;
    }
    if (mcr[group - 1] & MCR_LOOP) {
        loopc[group - 1] = v;
        loop[group - 1] = 1;
    } else {
        write(terminals[group - 1].outfd, &v, 1);
    }
    if (trace & trace_uart) printf("uart: write txb %s\n", printable(v));
}

static char *w_inte_bits[] = { "READAVAIL", "TXHOLDEMPTY", "RLINESTAT", "MDMSTAT", 0, 0, 0, 0 };

static void
wr_inte(portaddr p, byte v)
{
    if (dlab[group - 1]) {
        dlm[group - 1] = v;
        return;
    }
    if (trace & trace_uart) {
        printf("uart: write inte %x %s\n", v, bitdef(v, w_inte_bits));
    }
}

static char *w_linec_bits[] = { "WLS0", "WLS1", "STB", "PEN", "EPS", "STP", "SBRK", "DLAB" };

static void
wr_linectl(portaddr p, byte v)
{
    if (v & LCR_DLAB) {
        dlab[group - 1] = 1;
    } else {    
        dlab[group - 1] = 0;
    }
    if (trace & trace_uart) printf("uart: write linectl %x %s\n", v, bitdef(v, w_linec_bits));
}

static char *w_lines_bits[] = { "DR", "OE", "PE", "FE", "BI", "THRE", "TEMT", 0 };

static void
wr_linestat(portaddr p, byte v)
{
    if (trace & trace_uart) printf("uart: write linestat %x %s\n", v, bitdef(v, w_lines_bits));
}

static char *w_mdmc_bits[] = { "DTR", "RTS", "OUT1", "OUT2", "LOOP", 0, 0, 0 };

static void
wr_mdmctl(portaddr p, byte v)
{
    if ((v & MCR_LOOP) && !(mcr[group - 1] & MCR_LOOP)) {
        loop[group - 1] = 0;
    }
    mcr[group - 1] = v;
    if (trace & trace_uart) printf("uart: write mdmctl %x %s\n", v, bitdef(v, w_mdmc_bits));
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

static char *gsel_bits[] = { 0, 0, "membank", "intena", "printreset", "parstb", 0, 0 };

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

    if (trace & trace_multio) printf("multio: write group select %x %s\n", group, bitdef(v, gsel_bits));


    lastgroup = group;

    switch (group) {
    case 0:
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
    case 1:
    case 2:
    case 3:
        register_input(MULTIO_PORT + 0, &rd_rxb);
        register_input(MULTIO_PORT + 1, &rd_inte);
        register_input(MULTIO_PORT + 2, &rd_inti);
        register_input(MULTIO_PORT + 3, &rd_linectl);
        register_input(MULTIO_PORT + 4, &rd_mdmctl);
        register_input(MULTIO_PORT + 5, &rd_linestat);
        register_input(MULTIO_PORT + 6, &rd_mdmstat);

        register_output(MULTIO_PORT + 0, &wr_txb);
        register_output(MULTIO_PORT + 1, &wr_inte);
        register_output(MULTIO_PORT + 2, &undef_outreg);
        register_output(MULTIO_PORT + 3, &wr_linectl);
        register_output(MULTIO_PORT + 4, &wr_mdmctl);
        register_output(MULTIO_PORT + 5, &undef_outreg);
        register_output(MULTIO_PORT + 6, &undef_outreg);
        break;
    }
}

/*
psend(char *s)
{
    while (*s) {
        wr_txb(0, *s++);
    }
}
*/

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
sigalrm_handler(int sig)
{
    set_interrupt(vi_7, int_set);
}

void
sigio_handler(int sig)
{
    int i;
    int chars;
 
    for (i = 0; i < 3; i++) {
        if (terminals[i].infd == -1) {
            continue;
        }
        if (inte[i] & INTE_RDAV) {
            ioctl(terminals[i].infd, FIONREAD, &chars);
            if (chars) {
                set_interrupt(vi_3 + i, int_set);
            }
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

    atexit(exit_hook);

    for (i = 0; i < 3; i++) {
        terminals[i].infd = -1;
    }

    if (config_sw & CONF_SET) {
        termmask = (config_sw >> 8) & 0x7;
    }

    signal(SIGALRM, sigalrm_handler);
    signal(SIGIO, sigio_handler);

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
        terminals[0].infd = pipe_up[0];
        terminals[0].outfd = pipe_down[1];
    }

    // create xterms
    for (i = 0; i < 3; i++) {
        if (termmask & (1 << i)) {
            sprintf(tname, "multio-%d", i);
            open_terminal(tname, SIGIO, &terminals[i].infd, &terminals[i].outfd, 0, 0);
        }
    }

    terminal_fd_in = terminals[0].infd;
    terminal_fd_out = terminals[0].outfd;

	register_output(MULTIO_PORT+7, &multio_select);
    reg_intbit(vi_0, "hd");
    reg_intbit(vi_1, "djdma");
    reg_intbit(vi_3, "uart1");
    reg_intbit(vi_4, "uart2");
    reg_intbit(vi_5, "uart3");
    reg_intbit(vi_7, "clock");
    register_interrupt(vi_0, vi_handler);               // hdca and hddma
    register_interrupt(vi_1, vi_handler);               // djdma
    // register_interrupt(vi_2, vi_handler);               // unused
    register_interrupt(vi_3, vi_handler);               // uart
    register_interrupt(vi_4, vi_handler);               // uart
    register_interrupt(vi_5, vi_handler);               // uart
    // register_interrupt(vi_6, vi_handler);               // unused
    register_interrupt(vi_7, vi_handler);               // clock

    register_intvec(multio_intvec);

    return 0;
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
    register_startup_hook(multio_init);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
