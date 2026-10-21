/*
 * the network disk (ND) - the card behind sys/nd.c.
 *
 * A port-mapped block device in the HDCA's shape: three registers, a
 * four-byte block number and a 512-byte data area, served against the
 * backing image named by -N.  The card is one request at a time and does
 * each synchronously in the OUT that starts it, so a command that
 * completes and raises VI is what the kernel's probe tickles.
 *
 * The registers are the driver's (include/sys/nd.h):
 *
 *   0x64  read    op-done (0x01) and error (0x02)
 *   0x65  write   the command: ND_BLOCK/ND_RAMDATA/ND_READ/ND_WRITE
 *   0x67  read/write  the byte at the current pointer
 *
 * ND_ACK is the clear: it drops op-done, error and the line, and the
 * attach loop calls it either side of the tickle.  ND_BLOCK drops them
 * too, because it starts a new cycle.  A card with no backing image
 * still takes every command and still raises the line - it is the board
 * the probe tests for, not the medium - and the request completes with
 * the error bit rather than hanging.
 */

#include "sim.h"
#include "hwsim.h"
#include "vimap.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#define ND_STATUS_PORT  0x64    /* read: op-done + error */
#define ND_CMD_PORT     0x65    /* write: a command */
#define ND_DATA_PORT    0x67    /* read/write: a byte at the pointer */

#define ND_BLOCK        0x00
#define ND_RAMDATA      0x01
#define ND_READ         0x02
#define ND_WRITE        0x03
#define ND_ACK          0x04

#define ND_OPDONE       0x01
#define ND_ERROR        0x02

#define ND_CARDID       0
#define ND_BLKSZ        512

static int nd_vi_line = -1;     /* the VI line, from the -w wiring; -1 = not wired */
static int nd_fd = -1;          /* the backing image, -1 = no medium */
static int nd_done, nd_err;     /* the status latch */
static int nd_ptr;              /* 0 = block number, 1 = data */
static int nd_nb;               /* block-number bytes seen */
static unsigned long nd_blk;    /* the block asked for */
static unsigned char nd_buf[ND_BLKSZ];
static int nd_dpos;             /* the data pointer */

static void
nd_line()
{
    set_vi(nd_vi_line, ND_CARDID, nd_done ? 1 : 0);
}

static void
nd_cmd_out(portaddr p, byte v)
{
    ssize_t n;

    switch (v) {
    case ND_BLOCK:              /* a new cycle: the clear */
        nd_done = nd_err = 0;
        nd_ptr = 0;
        nd_nb = 0;
        nd_blk = 0;
        nd_dpos = 0;
        nd_line();
        return;
    case ND_RAMDATA:
        nd_ptr = 1;
        nd_dpos = 0;
        return;
    case ND_ACK:                /* the driver has taken the result */
        nd_done = nd_err = 0;
        nd_line();
        return;
    case ND_READ:
        nd_ptr = 1;
        nd_dpos = 0;
        memset(nd_buf, 0, ND_BLKSZ);
        nd_err = 0;
        if (nd_fd < 0) {
            nd_err = 1;
        } else {
            n = pread(nd_fd, nd_buf, ND_BLKSZ, (off_t) nd_blk * ND_BLKSZ);
            if (n != ND_BLKSZ)
                nd_err = 1;
        }
        nd_done = 1;
        nd_line();
        return;
    case ND_WRITE:
        nd_err = 0;
        if (nd_fd < 0) {
            nd_err = 1;
        } else {
            n = pwrite(nd_fd, nd_buf, ND_BLKSZ, (off_t) nd_blk * ND_BLKSZ);
            if (n != ND_BLKSZ)
                nd_err = 1;
        }
        nd_done = 1;
        nd_line();
        return;
    }
}

static void
nd_data_out(portaddr p, byte v)
{
    if (nd_ptr == 0) {
        if (nd_nb < 4) {
            nd_blk = (nd_blk << 8) | v;
            nd_nb++;
        }
        return;
    }
    if (nd_dpos < ND_BLKSZ)
        nd_buf[nd_dpos++] = v;
}

static byte
nd_data_in(portaddr p)
{
    byte b;

    b = 0;
    if (nd_ptr == 1 && nd_dpos < ND_BLKSZ)
        b = nd_buf[nd_dpos++];
    return b;
}

static byte
nd_status_in(portaddr p)
{
    return (nd_done ? ND_OPDONE : 0) | (nd_err ? ND_ERROR : 0);
}

static int
nd_startup()
{
    nd_vi_line = wiring_vi_line("nd");
    register_input(ND_STATUS_PORT, &nd_status_in);
    register_output(ND_CMD_PORT, &nd_cmd_out);
    register_input(ND_DATA_PORT, &nd_data_in);
    register_output(ND_DATA_PORT, &nd_data_out);

    if (nd_image)
        nd_fd = open(nd_image, O_RDWR);
    return 0;
}

struct driver nd_driver = {
    "nd",
    0,
    0,
    &nd_startup,
    0,
};

__attribute__((constructor))
void
register_nd_driver()
{
    register_driver(&nd_driver);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
