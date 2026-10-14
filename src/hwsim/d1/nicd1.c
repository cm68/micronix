/*
 * The network card (NET.md), d1 wiring.
 *
 * nic.c is the self-contained core; this is the d1 driver that hangs the
 * two ports on the S-100 bus and runs the periodic poll.  Interrupt on
 * RXRDY: the card asserts VI5 (the spare DISK3 line) whenever the RX FIFO
 * is non-empty and holds it until the guest drains it.
 */

#include "sim.h"
#include "hwsim.h"
#include "vimap.h"
#include "nic.h"

#define NETDATA_PORT 0x60    /* read/write: a byte of the channel */
#define NETSTAT_PORT 0x61    /* read: RXRDY/TXRDY/RDY */

static int nic_vi_line = -1;    /* the VI line, from the -w wiring; -1 = not wired */

/* assert the VI line while the RX FIFO holds a byte; drop it when it drains */
static void
nic_intr()
{
    set_vi(nic_vi_line, 0, (nic_status() & NETSTAT_RXRDY) ? 1 : 0);
}

static byte
net_data_in(portaddr p)
{
    byte b = nic_data_in();

    nic_intr();                 /* draining may have emptied the FIFO */
    return b;
}

static void
net_data_out(portaddr p, byte v)
{
    nic_data_out(v);
}

static byte
net_stat_in(portaddr p)
{
    return nic_status();
}

static void
nic_poll_cb(int a)
{
    nic_poll();
    nic_intr();                 /* arrived data may have filled the FIFO */
}

static int
nic_startup()
{
    nic_vi_line = wiring_vi_line("nic");
    register_input(NETDATA_PORT, &net_data_in);
    register_output(NETDATA_PORT, &net_data_out);
    register_input(NETSTAT_PORT, &net_stat_in);

    nic_init();
    recurring_time_out("nic", 100, &nic_poll_cb, 0);  /* poll host sockets at 100 Hz */
    return 0;
}

struct driver nic_driver = {
    "nic",
    0,               /* usage */
    0,               /* prearg */
    &nic_startup,    /* startup */
    0,               /* poll */
};

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_nic_driver()
{
    register_driver(&nic_driver);
}
