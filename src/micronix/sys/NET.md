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

A listener is persistent.  One connection may be parked on it; until the
guest `ACCEPT`s that connection the listener is paused and the tcp stack
refuses (RST) anything else, so there is exactly one pending connection
and no backlog to race over.  `ACCEPT` moves the parked connection to a
new socket of its own, which becomes `tcp-open`, and reopens the
listener; the listener keeps its port and stays `tcp-listen`.  A second
server (rshd, telnetd) has its own listener and can never see the
other's connection.  One connection per socket; to serve two clients at
once the guest accepts two.

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
    ACCEPT  0x0B  takes the parked connection; reply = status, new socket
    SOCKSTAT 0x0C card-level (socket 0xff); reply = socket table
    RECV    0x00  not a wire type: pull this socket's data, payload =
                  count[2] little-endian

`JOIN` puts the card on its network — the firmware does wifi associate +
DHCP — and the `ACK` carries the assigned `ip[4]` in network order; the
sim answers immediately with the host's address.  `RDY` stays clear until
the join has finished.

`RECV` is a pull, not a pushed stream: the guest names the socket and the
count it can take, and the card answers `DATA` with as much as it has, or
an `ACK` whose status is `no-data-yet` or `closed`.  Because only the
owner of a socket asks for that socket, two sessions cannot steal each
other's bytes.  `SOCKSTAT` returns one nine-byte record per listener or
connection — state, local port, peer address, peer port — which is what
netstat(1) prints.

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
carries the netconf payload.  `ACCEPT`'s carries status[1] and the new
socket[1]; no connection waiting is the error code `no-data-yet`.
`SOCKSTAT`'s carries the socket table.  `RECV` answers `DATA` or an
`ACK` (`no-data-yet`, `closed`).  `ACK` payload, for `STATUS`:

    state[1]  rx[2 little-endian]  tx[2 little-endian]

`rx` is bytes buffered waiting to read; `tx` is bytes queued to send.

`EVENT` payload:

    code[1]:  1 connected, 2 closed, 3 reset, 4 ready

`connected` also carries the peer's address and port, so a server can
authenticate the caller and netstat can show the connection without a
second command:

    code[1] ip[4] port[2]  connected; network order
    code[1]                closed, reset, ready

`ready` means the socket has something to pull; the card sends one per
socket until a `RECV` takes the data.  The card never pushes the data
itself.

The address is the one the connection was accepted from (the card's
`accept`), not a name; a `.rhosts` line matches the dotted quad.

Error codes (the `ACK` status):

    1 no-socket   2 bad-state   3 no-memory
    4 refused     5 timeout     6 closed

## 6. Flow control

**Receive.**  The card buffers what the network delivered — a udp
datagram, or tcp bytes up to its window — and pushes a one-byte `ready`
event; the guest pulls the bytes with `RECV`, only for the socket it
holds.  A slow guest leaves the data in the card, the tcp window shrinks,
and the sender backs off; udp datagrams beyond the buffer are dropped
(tftp's retry absorbs that).  End-to-end backpressure, no new mechanism.

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

The card's socket face is `/dev/net0`, char major 6, and it is a clone
node: `open` asks the card for the next free socket, and the file it
returns is that socket, named by the forwarded minor (slot + 1).  The
descriptor is the socket for everything after the open:

    read(fd, buf, n)        pull this socket's bytes
    write(fd, buf, n)       send on this socket
    ioctl(fd, NETLISTEN, 0) blocking: returns when a connection waits
    ioctl(fd, NETACCEPT, &peer)
                            takes it; the descriptor becomes the
                            connection and the same fd number is used
    ioctl(fd, NETBIND, &port), ioctl(fd, NETCONNECT, &addr)
    ioctl(fd, NETCARD, &req)
                            one card-level request (JOIN, GETCONF,
                            SETCONF, SOCKSTAT)

The request codes and the two structures are in `include/sys/net.h`.
`read` and `write` move at most 128 bytes a call and report end of file
as 0; `read` returns `EAGAIN` when the selector woke for another socket's
event, so `select(2)` is the wait (and an alarm bounds it when the card
has no interrupt line).  The card and the driver never share a pushed
frame stream, which is what makes concurrent sessions safe.

## 9. The three implementations

One contract, three bodies, the same guest binary:

    guest driver     builds the frames from the typed requests, pulls
                     per socket, and demuxes events by socket
    sim nic.c        models the two ports, translates frames to host sockets
    esp32 firmware   lwIP + this framing over the bus channel
