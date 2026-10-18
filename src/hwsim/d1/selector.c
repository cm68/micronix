/*
 * the CompuPro SELECTOR CHANNEL - the board that gives a temporary master
 * (the DISK 2) its bus cycles.  The host programs it with four bytes - the
 * 24 bit address, extended byte first, then the mode - and when the
 * controller takes the bus the channel generates the address and strobes.
 *
 * In this simulator the arbitration is not modelled cycle by cycle: the
 * channel is just the holder of the transfer's address and direction, and
 * the controller that uses it (disk2.c) reads them out when it moves data.
 * A read from the port disables the channel, which is how the byte count
 * is reset between transfers.
 */

#include "sim.h"
#include "hwsim.h"

#define SC_PORT  0xe0            /* the channel's port, switch selectable */

paddr sc_addr;                  /* the programmed 24 bit address */
byte sc_mode;                   /* the programmed mode byte */

static byte sc_bytes[4];
static int sc_n;

static byte
sc_in(portaddr p)
{
    sc_n = 0;                   /* a read disables the channel */
    return 0;
}

static void
sc_out(portaddr p, byte v)
{
    if (sc_n >= 4)
        sc_n = 0;
    sc_bytes[sc_n++] = v;
    if (sc_n == 4) {
        sc_addr = ((paddr) sc_bytes[0] << 16) | (sc_bytes[1] << 8) | sc_bytes[2];
        sc_mode = sc_bytes[3];
        sc_n = 0;
    }
}

static int
selector_init()
{
    register_input(SC_PORT, &sc_in);
    register_output(SC_PORT, &sc_out);
    return 0;
}

struct driver selector_driver = {
    "selector",
    0,
    0,
    &selector_init,
    0
};

__attribute__((constructor))
void
register_selector_driver()
{
    register_driver(&selector_driver);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
