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

/* port 0x4a */
static byte 
rd_clock(portaddr p)
{
    printf("multio: read clock\n");
}

/* port 0x4c */
static byte 
rd_pic_port_0(portaddr p)
{
    printf("multio: read pic0\n");
}

static byte 
rd_pic_port_1(portaddr p)
{
    printf("multio: read pic1\n");
}

static byte 
rd_rxb(portaddr p)
{
    byte retval;
    if (loop) {
        if (loop < 2) {
            printf("read unwritten loopback\n");
        }
        loop = 1;
        retval = loopc;
    } else {
        if (read(terminal_fd, &retval, 1) != 1) {
            printf("multio: rd_rxb failed\n");
        }
    }
    printf("multio: read rxb = %d\n", retval);
    return retval;
}

static byte 
rd_inte(portaddr p)
{
    printf("multio: read inte\n");
}

static byte 
rd_inti(portaddr p)
{
    printf("multio: read inti\n");
}

static byte 
rd_linectl(portaddr p)
{
    printf("multio: read linectl\n");
}

static byte 
rd_mdmctl(portaddr p)
{
    printf("multio: read mdmctl\n");
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
    printf("multio: read linestat %x\n", retval);
    return retval;
}

static byte 
rd_mdmstat(portaddr p)
{
    printf("multio: read mdmstat\n");
}

static void
wr_clock(portaddr p, byte v)
{
    printf("multio: write clock %x\n", v);
}

static void
wr_pic_port_0(portaddr p, byte v)
{
    printf("multio: write pic0 %x\n", v);
}

static void
wr_pic_port_1(portaddr p, byte v)
{
    printf("multio: write pic1 %x\n", v);
}

static void
wr_txb(portaddr p, byte v)
{
    if (loop) {
        loopc = v;
        loop = 2;
    } else {
        write(terminal_fd, &v, 1);
    }
    printf("multio: write txb %x %d %c\n", v, v, v);
}

static void
wr_inte(portaddr p, byte v)
{
    printf("multio: write inte %x\n", v);
}

static void
wr_linectl(portaddr p, byte v)
{
    printf("multio: write linectl %x\n", v);
}

static void
wr_linestat(portaddr p, byte v)
{
    printf("multio: write linestat %x\n", v);
}

static void
wr_mdmctl(portaddr p, byte v)
{
    if (v & MCR_LOOP) {
        loop = 1;
    } else {
        loop = 0;
    }
    printf("multio: write mdmctl %x\n", v);
}


/*
 * write the port select register
 */
void
multio_select(portaddr p, byte v)
{
    printf("multio: write group select %x\n", v);

    group = v & GROUP_MASK;
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

psend(char *s)
{
    while (*s) {
        wr_txb(0, *s++);
    }
}

extern char *mytty;

static int
multio_init()
{
    struct termios tio;

    terminal_fd = open(mytty, O_RDWR);
    if (terminal_fd == -1) {
        perror("terminal");
    }

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
    register_startup_hook(multio_init);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
