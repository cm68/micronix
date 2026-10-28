# Network interface — the card, and how the guest drives it

Out-of-band network I/O for the machine.  An ESP32 coprocessor (does all
of tcp/ip — lwIP — and joins wifi) sits behind a small bus-interface CPLD
and exposes one byte channel to the Z80.  The guest driver moves framed
commands and data through that channel; the card does the network.  This
is the same "a device that is really a door to a host-side service" idiom
as the media changer (MEDIACHG.md), but for sockets instead of floppies.

The register interface is ours — a CPLD, not a chip's register file — so
the sim and the card firmware implement exactly what is written here, and
the same guest binary runs on both.  Everything is debugged in the sim
first; the card is the honest target.

## 1. The card

An ESP32 + a bus-interface CPLD.  The Z80 sees two 8-bit S-100 ports:

    NETDATA   read: next byte from the RX FIFO
              write: a byte into the TX FIFO
    NETSTAT   read: status bits

`NETSTAT`:

    RXRDY  0x01   the RX FIFO holds a byte to read
    TXRDY  0x02   the TX FIFO has room for a byte
    RDY    0x04   the firmware is up

Interrupt-driven.  The card asserts a vectored-interrupt line (VI5, the
spare DISK3 line) whenever `RXRDY` is set and holds it until the RX FIFO
drains.  The guest handler masks the line and wakes the waiting ioctl,
which drains the FIFO and opens the line again for its next wait; the
handler never reads data.  The line is open-collector, so the card pulls it
through an open-collector driver (7406), not a push-pull pin.

The driver uses the line when the attach loop finds it, and polls
`NETSTAT` when it does not: a machine may have the card without a working
interrupt route, and frames and command replies are the same either way.
A polled driver has no wakeup to give `select(2)`, so a selector bounds
its wait with an alarm and re-checks `RXRDY` when the signal returns;
with a line attached the handler wakes it at once.

## 2. Framing

Both directions carry frames over the channel:

    [type:1][socket:1][len:2 little-endian][payload:len]

`socket` is 0–7 for a socket, or `0xFF` for a command addressed to the
card itself (section 4).  A frame's payload is at most 1024 bytes — a
512-byte disk block plus its request header (ND's write is 517, TFTP's
data is 516).

## 3. Sockets

Eight sockets, one connection each.  A socket is in one of six states:

    0  closed
    1  udp           open, ready to send/recv
    2  tcp-idle      open, neither connect nor listen yet
    3  tcp-listen    server, listening
    4  tcp-connect   client, connect issued, not yet established
    5  tcp-open      established, ready to send/recv

There is no `accept`: a listening socket becomes `tcp-open` when a client
arrives (the W5500 shape; keeps the state machine tiny).  One connection
per socket — to take a second connection the guest opens another socket.

## 4. Commands — guest to card

Each is a frame; the socket is in the header, arguments in the payload.

    OPEN    0x01  payload[0] = mode (0 udp, 1 tcp)
    CLOSE   0x02  no payload
    BIND    0x03  payload = port[2] network order
    CONNECT 0x04  payload = ip[4] port[2] network order
    LISTEN  0x05  no payload          (tcp server, after bind)
    SEND    0x06  payload = the bytes
    STATUS  0x07  no payload
    JOIN    0x08  card-level (socket 0xff), no payload
    GETCONF 0x09  card-level (socket 0xff), no payload
    SETCONF 0x0A  card-level (socket 0xff), payload = netconf

`JOIN` puts the card on its network — the firmware does wifi associate +
DHCP — and the `ACK` carries the assigned `ip[4]` in network order; the
sim answers immediately with the host's address.  `RDY` stays clear until
the join has finished.  There is no `recv`: the card pushes data
(section 5); the channel is the flow control.

`GETCONF` and `SETCONF` are the third way to give the card a network,
over this interface.  The netconf payload is the station's credentials:

    ssidlen[1]  ssid[ssidlen]  passlen[1]  pass[passlen]

`ssidlen` is 1-32; `passlen` is 0-63, and an empty password is an open
network.  `GETCONF`'s `ACK` carries that payload back.  `SETCONF` stores
the pair in the card's non-volatile memory and applies it at once: the
station drops its old association, reconnects with the new pair, and the
guest `JOIN`s again for the new address.  Both are card-level commands,
socket `0xff`, the same as `JOIN`.

The other two ways are the card's own setup access point and its
USB-serial editor; all three end in the same stored pair, and everything
after it — address, mask, gateway, DNS — is DHCP.

## 5. Pushed frames — card to guest

    ACK    0x80  answers the last command
    DATA   0x81  received bytes — one udp datagram, or a tcp chunk
    EVENT  0x82  an asynchronous change

`ACK` payload, for every command but `STATUS`, `JOIN` and `GETCONF`:

    status[1]:  0 ok, else the error code below

`JOIN`'s successful `ACK` carries `ip[4]` (section 4).  `GETCONF`'s
carries the netconf payload.  `ACK` payload, for `STATUS`:

    state[1]  rx[2 little-endian]  tx[2 little-endian]

`rx` is bytes buffered waiting to read; `tx` is bytes queued to send.

`EVENT` payload:

    code[1]:  1 connected, 2 closed, 3 reset

`connected` also carries the peer's address, so a server can authenticate
the caller without a second command:

    code[1] ip[4]          connected; ip in network order
    code[1]                closed, reset

The address is the one the connection was accepted from (the card's
`accept`), not a name; a `.rhosts` line matches the dotted quad.

Error codes (the `ACK` status):

    1 no-socket   2 bad-state   3 no-memory
    4 refused     5 timeout     6 closed

## 6. Flow control

**Receive.**  The card buffers what the network delivered — a udp
datagram, or tcp bytes up to its window — and pushes `DATA` frames; the
guest reads them when `RXRDY` is set.  A slow guest fills the card's rx
buffer, the tcp window shrinks, and the sender backs off; udp datagrams
beyond the buffer are dropped (tftp's retry absorbs that).  End-to-end
backpressure, no new mechanism.

**Send.**  Two levels.  The FIFO is the byte throttle: the guest writes
frame bytes only while `TXRDY` is set.  The card's memory is the frame
throttle: if a socket's tx buffer is full the card answers `ACK
no-memory` and the guest backs off, polling `STATUS`'s `tx` count.  A
`SEND`'s `ACK` means "accepted for transmission", not "sent".

## 7. The three faces

All three are thin guest layers over sockets; none touches tcp/ip.

**TFTP** — a udp socket.  RRQ/WRQ and the stop-and-wait DATA/ACK dance;
each 512-byte block is one `SEND`/`DATA`.  Stop-and-wait sits on top of
the channel's own flow control.

**ND** — a udp socket.  The block driver's `strat()` sends "read block N"
/ "write block N" as a datagram; the disk server answers with one
512-byte datagram.  The guest mounts the socket as a block device and its
whole disk i/o is this.

**rlogin/telnet** — a tcp-server socket.  `EVENT connected`, then a byte
stream of `DATA`/`SEND` frames, plumbed through a pty (separate work) to
a shell.

## 8. The guest interface

The card is one face of the ND module: `/dev/net` is block major 6,
minor 1 (`dev/devlist`), and every operation is one ioctl command block
(`include/sys/ioctl.h`):

    ioctl(fd, CDBCMD, &cdb)

`cdb.cmd[0]` is the operation (`N_OPEN` … `N_SETCONF`, or `NET_OP_RECV`
to take the next frame), `cdb.cmd[1]` the socket, and the operation's
own fields follow at `cdb.cmd[2..]`; `cdb.buf`/`cdb.count` are its data
phase.  The driver builds the frame from those fields, sends it, and
reads the reply before returning: `cdb.cmd[0..2]` carry the reply type,
socket and first payload byte, and `cdb.count` the payload copied back.
The operation list and field layout are in `include/sys/net.h`; a reply
payload needs `CDB_IN` and `count` set to the buffer's size.

A caller that owns the whole pushed-frame stream may set `CDB_ASYNC` on a
`SEND`.  The driver then does not read the reply: the ACK is pushed in
order with the DATA frames and the caller reads it like any other, which
is what lets rshd move bytes both ways from one process.  A caller that
waits for each ACK inline (rcp) leaves the flag clear.

## 9. The three implementations

One contract, three bodies, the same guest binary:

    guest driver     builds the frames from the ioctl's typed fields
    sim nic.c        models the two ports, translates frames to host sockets
    esp32 firmware   lwIP + this framing over the bus channel
