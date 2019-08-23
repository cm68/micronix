/*
 * the multio driver is both for the onboard I/O on the multio card
 * and the wunderbus I/O, which are roughly equivalent
 *
 * this has rtc, serial ports and an interrupt controller, 
 * and some parallel stuff
 */

#include "sim.h"
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>

int trace_multio;

int terminal_fd;

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
#define GRP2        0x02    // serial 1
#define GRP3        0x03    // serial 1

// linestat
#define LSR_DR      0x01    // input data ready
#define LSR_OVER    0x02    // input data overrun
#define LSR_PERR    0x04    // input parity error
#define LSR_FERR    0x08    // input framing error
#define LSR_BRK     0x10    // break
#define LSR_TBE     0x20    // transmit buffer empty
#define LSR_TE      0x40    // transmitte empty

// modem control register
#define MCR_DTR     0x01    // data terminal ready
#define MCR_RTS     0x02    // request to send
#define MCR_LOOP    0x10    // loopback

// line control register
#define LCR_82      0x07    // 8 data bits, 2 stop bits
#define LCR_DLAB    0x80    // accessio baud in inte, txb

static byte group;
static byte loop;
static byte loopc;

/* read/write baud rate */
static byte dlab;
static byte dll;
static byte dlm;

/* port 0x4a */
static byte 
rd_clock(portaddr p)
{
    if (trace & trace_multio) printf("multio: read clock\n");
    return 0;
}

/* port 0x4c */
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

static byte 
rd_rxb(portaddr p)
{
    byte retval;
    int bytes;

    if (dlab) {
        return dll;
    }
    if (loop) {
        if (loop < 2) {
            if (trace & trace_multio) printf("read unwritten loopback\n");
        }
        loop = 1;
        retval = loopc;
    } else {
        ioctl(terminal_fd, FIONREAD, &bytes);
        if (bytes) {
            if (read(terminal_fd, &retval, 1) != 1) {
                if (trace & trace_multio) printf("multio: rd_rxb failed\n");
            }
        } else {
            retval = 0;
        }
    }
    if (trace & trace_multio) printf("multio: read rxb = %d\n", retval);
    return retval;
}

static byte 
rd_inte(portaddr p)
{
    if (dlab) {
        return dlm;
    }
    if (trace & trace_multio) printf("multio: read inte\n");
    return 0;
}

static byte 
rd_inti(portaddr p)
{
    if (trace & trace_multio) printf("multio: read inti\n");
    return 0;
}

static byte 
rd_linectl(portaddr p)
{
    if (trace & trace_multio) printf("multio: read linectl\n");
    return 0;
}

static byte 
rd_mdmctl(portaddr p)
{
    if (trace & trace_multio) printf("multio: read mdmctl\n");
    return 0;
}

static byte 
rd_linestat(portaddr p)
{
    int bytes = 0;
    byte retval;

    if (loop) {
        if (loop == 2) {
            bytes = 1;
        } else {
            bytes = 0;
        }
    } else {
        ioctl(terminal_fd, FIONREAD, &bytes);
    }

    retval = LSR_TBE | (bytes ? LSR_DR : 0);
    // printf("multio: read linestat %x\n", retval);
    return retval;
}

static byte 
rd_mdmstat(portaddr p)
{
    if (trace & trace_multio) printf("multio: read mdmstat\n");
    return 0;
}

static void
wr_clock(portaddr p, byte v)
{
    if (trace & trace_multio) printf("multio: write clock %x\n", v);
}

static void
wr_pic_port_0(portaddr p, byte v)
{
    if (trace & trace_multio) printf("multio: write pic0 %x\n", v);
}

static void
wr_pic_port_1(portaddr p, byte v)
{
    if (trace & trace_multio) printf("multio: write pic1 %x\n", v);
}

static void
wr_txb(portaddr p, byte v)
{
    if (dlab) {
        dll = v;
        return;
    }
    if (loop) {
        loopc = v;
        loop = 2;
    } else {
        write(terminal_fd, &v, 1);
    }
    if (trace & trace_multio) printf("multio: write txb %x %d %c\n", v, v, v);
}

static char *w_inte_bits[] = { "READAVAIL", "TXHOLDEMPTY", "RLINESTAT", "MDMSTAT", 0, 0, 0, 0 };

static void
wr_inte(portaddr p, byte v)
{
    if (dlab) {
        dlm = v;
        return;
    }
    if (trace & trace_multio) {
        printf("multio: write inte %x %s\n", v, bitdef(v, w_inte_bits));
    }
}

static char *w_linec_bits[] = { "WLS0", "WLS1", "STB", "PEN", "EPS", "STP", "SBRK", "DLAB" };

static void
wr_linectl(portaddr p, byte v)
{
    if (v & LCR_DLAB) {
        dlab = 1;
    } else {    
        dlab = 0;
    }
    if (trace & trace_multio) printf("multio: write linectl %x %s\n", v, bitdef(v, w_linec_bits));
}

static char *w_lines_bits[] = { "DR", "OE", "PE", "FE", "BI", "THRE", "TEMT", 0 };

static void
wr_linestat(portaddr p, byte v)
{
    if (trace & trace_multio) printf("multio: write linestat %x %s\n", v, bitdef(v, w_lines_bits));
}

static char *w_mdmc_bits[] = { "DTR", "RTS", "OUT1", "OUT2", "LOOP", 0, 0, 0 };

static void
wr_mdmctl(portaddr p, byte v)
{
    if (v & MCR_LOOP) {
        loop = 1;
    } else {
        loop = 0;
    }
    if (trace & trace_multio) printf("multio: write mdmctl %x %s\n", v, bitdef(v, w_mdmc_bits));
}

/*
 * write the port select register
 */
void
multio_select(portaddr p, byte v)
{
    static int lastgroup = -1;

    group = v & GROUP_MASK;
    if (trace & trace_multio) printf("multio: write group select %x\n", group);

    if (group == lastgroup)
        return;

    lastgroup = group;

    switch (group) {
    case 0:
        register_input(MULTIO_PORT + 2, &rd_clock);
        register_input(MULTIO_PORT + 4, &rd_pic_port_0);
        register_input(MULTIO_PORT + 5, &rd_pic_port_1);
        register_output(MULTIO_PORT + 2, &wr_clock);
        register_output(MULTIO_PORT + 4, &wr_pic_port_0);
        register_output(MULTIO_PORT + 5, &wr_pic_port_1);
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
        register_output(MULTIO_PORT + 3, &wr_linectl);
        register_output(MULTIO_PORT + 4, &wr_mdmctl);
        register_output(MULTIO_PORT + 5, &wr_linestat);
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

extern char *mytty;
static struct termios original_tio;

static void
exit_hook()
{
    tcsetattr(terminal_fd, TCSANOW, &original_tio);
}

static int
multio_init()
{
    struct termios tio;

    atexit(exit_hook);

    terminal_fd = open(mytty, O_RDWR);
    if (terminal_fd == -1) {
        perror("terminal");
    }

    tcgetattr(terminal_fd, &original_tio);
    tcgetattr(terminal_fd, &tio);
    cfmakeraw(&tio);
    tcsetattr(terminal_fd, TCSANOW, &tio);

	register_output(MULTIO_PORT+7, &multio_select);
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
    trace_multio = register_trace("multio");
    register_startup_hook(multio_init);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
