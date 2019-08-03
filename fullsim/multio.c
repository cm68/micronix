/*
 * the multio driver is both for the onboard I/O on the multio card
 * and the wunderbus I/O, which are roughly equivalent
 *
 * this has rtc, serial ports and an interrupt controller, 
 * and some parallel stuff
 */

#include "sim.h"

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

static byte group;

/* port 0x4a */
static byte 
rd_clock(portaddr p)
{
    printf("read clock\n");
}

/* port 0x4c */
static byte 
rd_pic_port_0(portaddr p)
{
    printf("read pic0\n");
}

static byte 
rd_pic_port_1(portaddr p)
{
    printf("read pic1\n");
}

static byte 
rd_rxb(portaddr p)
{
    printf("read rxb\n");
}

static byte 
rd_inte(portaddr p)
{
    printf("read inte\n");
}

static byte 
rd_inti(portaddr p)
{
    printf("read inti\n");
}

static byte 
rd_linectl(portaddr p)
{
    printf("read linectl\n");
}

static byte 
rd_mdmctl(portaddr p)
{
    printf("read mdmctl\n");
}

static byte 
rd_linestat(portaddr p)
{
    printf("read linestat\n");
}

static byte 
rd_mdmstat(portaddr p)
{
    printf("read mdmstat\n");
}

static void
wr_clock(portaddr p, byte v)
{
    printf("write clock %x\n", v);
}

static void
wr_pic_port_0(portaddr p, byte v)
{
    printf("write pic0 %x\n", v);
}

static void
wr_pic_port_1(portaddr p, byte v)
{
    printf("write pic1 %x\n", v);
}

static void
wr_txb(portaddr p, byte v)
{
    printf("write txb %x\n", v);
}

static void
wr_inte(portaddr p, byte v)
{
    printf("write inte %x\n", v);
}

static void
wr_linectl(portaddr p, byte v)
{
    printf("write linectl %x\n", v);
}

static void
wr_mdmctl(portaddr p, byte v)
{
    printf("write mdmctl %x\n", v);
}


/*
 * write the port select register
 */
void
multio_select(portaddr p, byte v)
{
    printf("write group select %x\n", v);

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
        break;
    }
}

static int
multio_init()
{
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
