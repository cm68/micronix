/*
 * Network card driver (NET.md), the guest's framing half.
 *
 * sys/net.c
 *
 * A frame-level channel over the card's two ports.  write() pushes a
 * frame's bytes into NETDATA for the card to execute; read() assembles
 * one pushed frame (ACK/DATA/EVENT) and copies it back.  The socket is
 * in the frame's own header, so one device (/dev/net) carries all eight.
 * The socket logic - join, open, connect, the ftp conversation - is the
 * caller's; this only moves frames.
 *
 * Both directions are polled (NET.md: the interrupt line is not part of
 * the contract), so a read busy-waits on RXRDY.  That is the one place
 * this driver can block the machine, and it wants a timeout before it is
 * honest; for now the caller reads only when a frame is expected.
 */

#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/net.h>

netopen(dev, mode)
{
    return (0);                 /* the card is always there */
}

netclose(dev, mode)
{
    return (0);
}

/*
 * Assemble one pushed frame and copy it to the caller: the 4-byte header
 * (type, socket, len) then the payload, up to the caller's count.  The
 * frame is always consumed whole even if the caller's buffer is shorter
 * than it; what does not fit is dropped.
 */
netread(dev)
{
    UINT8 hdr[4];
    int len, i;
    UINT8 b;

    if (u.count == 0)
        return;

    while (!(in(NETSTAT_PORT) & NETSTAT_RXRDY))
        ;                       /* TODO: sleep + wake on the VI5 interrupt
                                 * (NET.md).  Busy-waiting here blocks the
                                 * whole machine; becomes an interrupt
                                 * driver once the rework lands. */

    for (i = 0; i < 4; i++)
        hdr[i] = in(NETDATA_PORT);
    len = hdr[2] | (hdr[3] << 8);

    for (i = 0; i < 4 && u.count; i++) {
        putbyte(hdr[i], u.base);
        u.base++;
        u.count--;
    }
    for (i = 0; i < len; i++) {
        b = in(NETDATA_PORT);
        if (u.count) {
            putbyte(b, u.base);
            u.base++;
            u.count--;
        }
    }
}

/*
 * Push a frame's bytes into NETDATA.  The caller already laid them out as
 * type, socket, len, payload; this is the byte throttle only.
 */
netwrite(dev)
{
    while (u.count) {
        while (!(in(NETSTAT_PORT) & NETSTAT_TXRDY))
            ;
        out(NETDATA_PORT, getbyte(u.base));
        u.base++;
        u.count--;
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
