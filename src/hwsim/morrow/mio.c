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

extern int_level int_s100;

int trace_multio;
int trace_noclock;
int trace_intr;

#define HZ_1    0x800

#define	MULTIO_MASTER	0x48	// multio master standard port
#define	MULTIO_SLAVE	0x58	// multio slave standard port

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

#define IRQ0    0x01        // vi0 - hddma/hdca
#define IRQ1    0x02        // vi1 - djdma
#define IRQ2    0x04        // vi2
#define IRQ3    0x08        // uart 1
#define IRQ4    0x10        // uart 2
#define IRQ5    0x20        // uart 3
#define IRQ6    0x40        // daisy
#define IRQ7    0x80        // clock interrupt

byte intin;         // actual interrupt line on s100

void
reg_intbit(int_line signal, char *name)
{
    intbits[signal] = name;
}

int
multio_dump(char **p)
{
}

/*
 * set the level of the interrupt line
 * this set irr in the interrupt controller and possibly sets the s100 
 * interrupt line the s100 interrupt line is reset by the interrupt 
 * vector generation.
 */
void
vi_handler(int_line signal, int_level level)
{
    byte mask;

    if (trace & trace_multio) {
#ifdef notdef
        printf("multio: vi_handler %d %s %s\n", 
            signal, intbits[signal], (level == int_set) ? "set" : "clear");
#endif
    }

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

    // if no state change, don't do anything
    if ((level == int_clear) && !(irq & mask)) return;
    if ((level == int_set) && (irq & mask)) return;

    if (level == int_set) {
        // if level, unconditional. if edge, need false initial state
        if ((icw1 & ICW1_LTIM) || (!(irq & mask))) {
            irr |= mask;
        }
        irq |= mask;
    } else {
        irr &= ~mask;
        irq &= ~mask;
    }

    // if any unmasked interrupts are high, assert int
    if (irr & ~(imr | isr)) {
        intin = 1;
        set_interrupt(interrupt, int_set);
    } else {
        intin = 0;
        set_interrupt(interrupt, int_clear);
    }

    if (trace & trace_intr) {
#ifdef notdef
        printf("intr: vi_%d %s %s intin: %s\n", 
            signal, (level == int_set) ? "set" : "clear", intbits[signal], intin ? "set" : "clear");
#else
        message("multio: vi_handler ");
        message(intbits[signal]);
        message((level == int_set) ? " set\n" : " clear\n");
#endif
    }
}


// run an interrupt ack cycle and return the vector byte(s) to the cpu card
intvec
multio_getvector()
{
    int level;
    int found;
    byte mask;
    intvec iv;

    if (trace & trace_multio) {
        printf("multio_getvector:\n");
        multio_dump(0);
    }
    /*
     * find the interrupt request line, starting from the current highest
     * priority
     */
    found = -1;
    level = priority;
    do {
        mask = 1 << level;

        // if we have an unmasked request, we found our request
        if ((mask & irr) && !(mask & imr)) {
            found = level;
            break;
        }
        level = (level + 1) % 8;
    } while (level != priority);

    // we didn't find a cause for our interrupt
    if (found == -1) {
        printf("lose: no unmasked request found!\n");
        multio_dump(0);
        return 0;
    }

    // if isr is set, then we are already servicing an interrupt
    isr |= mask; 
    iv = INTA_LEN | (VECTOR(found) << 8) | 0xcd;
    irr &= ~mask;

    if (irr & ~(imr | isr)) {
        intin = 1;
        set_interrupt(interrupt, int_set);
    } else {
        intin = 0;
        set_interrupt(interrupt, int_clear);
    }

    if (trace & trace_multio) printf("multio: vector %d (%s) multio_intvec %08x\n",
         found, intbits[found], iv);
    if (trace & trace_intr) {
        printf("vectoring to %02x %s handler at 0x%04x\n", 
            1 << found, intbits[found], VECTOR(found));
    }
    return (iv);
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
        printf("multio: write clock %02x %s %s\n", 
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
                recurring_timeout("multio_clock", rate, clock_handler, 0);
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
    if (trace & trace_multio) printf("multio: read clock %02x\n", v);
    return v;
}

static byte
rd_daisy(portaddr p)
{
    byte ret;

    ret = 0;
    if (trace & trace_multio) printf("multio: read daisy %02x = 0x%02x\n", p, ret);
    return ret;
}

static void
wr_daisy0(portaddr p, byte v)
{
    if (trace & trace_multio) printf("multio: write daisy0 %02x\n", v);
}

static void
wr_daisy1(portaddr p, byte v)
{
    if (trace & trace_multio) printf("multio: write daisy1 %02x\n", v);
}

static byte
undef_inreg(portaddr p)
{
    if (trace & trace_multio) printf("multio: read to %02x+%02x undefined\n", MULTIO_MASTER, p - MULTIO_MASTER);
    return 0;
}

static void
undef_outreg(portaddr p, byte v)
{
    if (trace & trace_multio) printf("multio: write 0x%02x to undefined %02x+%02x\n", v, MULTIO_MASTER, p - MULTIO_MASTER);
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
        printf("multio: write group select %02x %s\n", group, bitdef(v, group_bits));
    }

    switch (group) {
    case GRP0:  // parallel port, clock, pic
        register_input(MULTIO_MASTER + 0, &rd_daisy);
        register_input(MULTIO_MASTER + 1, &undef_inreg);
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
        acep = &ace[group - 1];
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

void
sigio_handler(int sig)
{
    int i;
 
    for (i = 0; i < 3; i++) {
        if (ace[i].infd == -1) {
            continue;
        }
        uart_interrupt_check(i);
    }
}

static int
register_multio(int slave)
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

    register_1990(vi_7);

    mysignal(SIGIO, sigio_handler);

    for (i = 0; i < 3; i++) {
        sprintf(tname, "uart%d", i);
        ace_init(i, strdup(tname), vi_3 + i);
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

	register_output(MULTIO_MASTER+7, &multio_select);
	register_output(MULTIO_SLAVE+7, &multio_slave_select);
    reg_intbit(vi_0, "hd");
    reg_intbit(vi_1, "djdma");
    reg_intbit(vi_2, "slave");
    reg_intbit(vi_7, "clock");
    register_interrupt(vi_0, vi_handler);               // hdca and hddma
    register_interrupt(vi_1, vi_handler);               // djdma
    register_interrupt(vi_2, vi_handler);               // slave
    register_interrupt(vi_7, vi_handler);               // clock

    register_intvec(multio_getvector);
    register_mon_cmd('M', "\tdump multio state", multio_dump);

    return 0;
}

void
init_multio()
{
}

void
usage_multio()
{
}

__attribute__((constructor))    // call before main()
void
register_multio_driver()
{
    trace_multio = register_trace("multio");
    trace_intr = register_trace("intr");
    register_driver(init_multio);
    register_startup_hook(start_multio);
    register_usage_hook(usage_multio);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
