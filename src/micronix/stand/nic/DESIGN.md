# The ESP32 network card firmware — design

`src/main.cpp` is the body.  This file is the intent behind it: what the
firmware is for, the wire protocol it speaks, the VI5 interrupt
handshake, and how the two tasks are put together.

The contract is `src/micronix/sys/NET.md`.  The golden reference for the
card side is the simulator's model, `src/hwsim/d1/nic.c`; the guest
driver is `src/micronix/sys/net.c`; the board and timing are
`hw/esp32-nic.md`.  Provisioning (access point, serial editor, `netconf`)
is described in `README.md`; the constants shared with the guest are in
`include/netproto.h`.

## 1. Intent

The card is a **door to a host-side service**, not a bus controller.
The Z80 sees two byte-wide I/O ports and one open-collector interrupt;
everything above the byte channel — framing, sockets, TCP/IP, Wi-Fi —
belongs to the ESP32.  That is the same division as the simulator's
card model, so the guest binary is unchanged between the two.

The firmware is deliberately **thin at the bus edge and thick behind
it**:

- one core runs a poll loop with no allocation, no logging, no flash
  access and no yielding; it only moves bytes between the bus and two
  FIFOs;
- the other core runs the Wi-Fi stack and the socket state machine at
  100 Hz, the same cadence as `nic.c`.

The target is a **6 MHz Z80**.  At that clock an I/O write window is
about 250 ns, too short to trust a firmware read of the data bus.  The
board therefore latches DO in a 74LS373 (`U23`), with
`LE = sOUTb AND SEL*`, and this firmware reads the held byte on the
falling edge of `sOUT`.  The latch is the timing guarantee; the loop is
not racing the CPU.  Reads are pre-driven: the ESP32 keeps DI set to
the byte a read would return, and the read strobe only advances the
receive FIFO.

Parity with `nic.c` is a design constraint, not a coincidence.  The
command set, socket states, ACK/EVENT codes and flow-control behaviour
are the same, so a guest that works under `d1`/`usersim` runs on the
card without a second protocol.

Non-goals: the firmware does not implement a bus master, DMA, the ND
block card, or a per-socket receive buffer.  It does not accept more
than one connection per socket, and it has no `accept` command.

## 2. Protocols

### 2.1 The byte channel

| Port | Address | Read | Write |
|---|---|---|---|
| NETDATA | 0x60 | next card-to-guest byte | one guest-to-card byte |
| NETSTAT | 0x61 | status bits | ignored |

`NETSTAT`: `RXRDY 0x01` (the rx FIFO has a byte), `TXRDY 0x02` (the tx
FIFO has room), `RDY 0x04` (the station is up and `JOIN` has completed).

Frames are carried whole in both directions:

    [type:1][socket:1][len:2 little-endian][payload:len]

`socket` is 0–7, or `0xFF` for a card-level command.  The payload is at
most 1024 bytes.  A frame longer than that is discarded with
`ACK bad-state`; the assembler never overruns its buffer.

### 2.2 Guest to card

| Type | Name | Socket | Payload | Reply |
|---|---|---|---|---|
| 0x01 | OPEN | 0–7 | mode: 0 UDP, 1 TCP | ACK |
| 0x02 | CLOSE | 0–7 | — | ACK (a bad slot gets no ACK) |
| 0x03 | BIND | 0–7 | port[2] network order | ACK |
| 0x04 | CONNECT | 0–7 | ip[4] port[2] network order | ACK; TCP also EVENT |
| 0x05 | LISTEN | 0–7 | — (bind first) | ACK |
| 0x06 | SEND | 0–7 | bytes | ACK: accepted, not sent |
| 0x07 | STATUS | 0–7 | — | ACK: state, rx[2], tx[2] |
| 0x08 | JOIN | 0xFF | — | ACK: ip[4], or timeout |
| 0x09 | GETCONF | 0xFF | — | ACK: len ssid, ssid, len pass, pass |
| 0x0A | SETCONF | 0xFF | len ssid, ssid, len pass, pass | ACK |

`CONNECT` on a TCP socket ACKs immediately, before the association is
attempted; the result arrives as `EVENT connected` or
`EVENT reset`.  UDP `CONNECT` only fixes the peer used by later `SEND`s.

`SEND` on TCP may be answered before all bytes are written; a partial
write keeps at most one 1024-byte backlog in `txbuf` and the network
task drains it.  A second `SEND` while that backlog exists gets
`no-memory`.  On UDP a `SEND` is one datagram to the connected peer.

`JOIN` is idempotent once joined: it answers with the station address
again.  With no credentials it starts the setup access point and
answers `timeout`.  While a join is in progress there is no immediate
answer; the network task sends the ACK with the address, or `timeout`
after 30 seconds.

### 2.3 Card to guest

| Type | Name | Payload |
|---|---|---|
| 0x80 | ACK | status[1]; or STATUS's five bytes; or JOIN's ip[4]; or GETCONF's configuration |
| 0x81 | DATA | received bytes: one UDP datagram, or a TCP chunk |
| 0x82 | EVENT | code[1] |

ACK status: 0 ok, 1 no-socket, 2 bad-state, 3 no-memory, 4 refused,
5 timeout, 6 closed.  EVENT codes: 1 connected, 2 closed, 3 reset.

### 2.4 Socket states

    0 closed   1 udp   2 tcp-idle   3 tcp-listen   4 tcp-connect   5 tcp-open

UDP OPEN goes to `udp`; TCP OPEN goes to `tcp-idle`.  BIND moves a UDP
socket's port; LISTEN requires a bound port and goes to `tcp-listen`.
CONNECT on TCP goes to `tcp-connect`, then `tcp-open` or `closed`.
A listening socket accepts one client and becomes `tcp-open`; the
listener is closed, so there is one connection per socket and no
`accept`.

### 2.5 Flow control

**Receive.**  The card pushes `DATA` frames into an 8 KB ring.  The
guest reads when `RXRDY` is set.  A slow guest fills the ring; TCP's
window shrinks, and UDP datagrams that do not fit are dropped (TFTP's
retry absorbs that).  There is no per-socket receive buffer: `STATUS`'s
rx count is always 0, exactly as `nic.c` reports it.

**Send.**  The guest writes only while `TXRDY` is set; the 4 KB tx ring
is the byte throttle.  A `SEND`'s ACK means "accepted", not "on the
wire".  The socket's own backlog is the second level, as above.

**Interrupt.**  VI5 follows the receive ring; section 3 is the
handshake.

### 2.6 Provisioning protocols

All three paths store the same pair in NVS (`Preferences`, namespace
`nic`, keys `ssid` and `pass`) and survive a firmware flash.

- **Access point.**  No credentials: an AP named `micronix-nic-xxxx`
  (last two MAC bytes), open unless `NIC_AP_PASS` is set, with a
  captive DNS.  `http://192.168.4.1/` takes the pair; `/save` stores and
  restarts, `/reset` forgets them.  A provisioned network that does not
  answer within 20 seconds brings the AP back in AP+STA mode.
- **Serial console.**  115200.  Any character starts a scan and an
  editable SSID/password prompt; Enter writes NVS and, if anything
  changed, restarts.
- **Guest interface.**  `GETCONF` (0x09) and `SETCONF` (0x0A) over
  `/dev/net`, driven by `netconf`.  `SETCONF` writes NVS, drops the old
  association and reconnects immediately; the guest then `JOIN`s again.

The station runs under DHCP and registers the hostname and mDNS name
`micronix-nic-xxxx.local`.  The compile-time `secrets.h` or
`-DWIFI_SSID=...` pair is only a bench fallback when NVS is empty.

## 3. Interrupt architecture

The card has one interrupt, `VI5` (S-100 edge pin 9).  It is active low,
level-asserted and open-collector.  GPIO12 drives a 7406 inverter; the
7406's open-collector output pulls VI5 low.  **GPIO12 high asserts the
line; GPIO12 low releases it.**  A 10 kΩ pull-down on GPIO12 keeps VI5
released at reset.

The line reflects the receive FIFO, not an edge event:

    netTask:   push whole frame -> rx_head release-store
               irqMux { if (!asserted) GPIO12 high, asserted = true }

    busTask:   NETDATA read -> rx_get() one byte
               if (rx empty)
                 irqMux { if (rx still empty) GPIO12 low, asserted = false }

Both sides touch the line under the same spinlock (`irqMux`) and both
re-test the condition inside it.  Whichever runs second sees the
other's state: a frame queued concurrently cannot be missed, and the
line cannot be left low with data waiting.  Assertion happens only
after a **whole frame** is in the ring, so the guest can read a frame
without the line dropping in the middle; the line stays low until the
ring is empty, including any frames that follow.

The line is level-asserted, so the guest releases it by draining
`NETDATA`, not by acknowledging an edge.  That is why the producer does
not need to know whether the guest is in its handler.

The current guest driver (`sys/net.c`) polls `RXRDY` and does not claim
VI5.  The firmware implements the contract anyway; until the guest
driver attaches the line (`sys/INTR.md`), a hardware bring-up should
keep VI5 masked or released, because an unclaimed level-asserted line
stays low for as long as data is waiting.  The simulator's card model
asserts the same line, so the guest-side change is the remaining piece.

## 4. Implementation

### 4.1 Two tasks

| Task | Core | Priority | Work |
|---|---|---|---|
| `busTask` | 1 | `configMAX_PRIORITIES - 1` | poll the bus, move bytes, follow the FIFO with VI5 |
| `netTask` | 0 | 5 | frame assembly, sockets, Wi-Fi, provisioning, 100 Hz |

`busTask` is `IRAM_ATTR`, never yields, and does not allocate, log or
touch flash.  Core 1's task watchdog is disabled because that loop
deliberately starves its idle task.  `netTask` sleeps 10 ms per pass,
matching `nic.c`'s poll rate.

### 4.2 The bus loop

Each pass reads `GPIO_IN_REG` and `GPIO_IN1_REG` once and decodes:

- `SEL*` — the 74LS682's `/P=Q`, **active low** (`SEL_ACTIVE = false`);
- `A0` — high selects NETSTAT (0x61);
- `sINP` and `sOUT` — active high S-100 strobes.

**Reads.**  The loop continuously drives DI with a precomputed
set/clear mask for the byte a read would return: the rx head for
NETDATA, `status_byte()` for NETSTAT.  On the **leading edge** of
`sINP` for NETDATA it calls `rx_get()`; if that empties the ring it
releases VI5 under `irqMux`.

**Writes.**  `A0` is captured at the **leading edge** of `sOUT`, while
the address is still valid.  On the **falling edge** the loop reads the
byte from the 74LS373's Q outputs (through U20) and puts it in the tx
ring.  The latch is transparent during `sOUT` and holds on the fall, so
the data is stable whether the loop reads it at the edge or later.

The pin map is in `README.md`; the board and the latch are in
`hw/esp32-nic.md`.

### 4.3 The FIFOs

Both are single-producer/single-consumer rings with free-running
indices masked to the buffer size:

| Ring | Size | Producer | Consumer |
|---|---|---|---|
| `rxq` | 8192 | `netTask` (frames to the guest) | `busTask` (NETDATA reads) |
| `txq` | 4096 | `busTask` (guest writes) | `netTask` (frame assembly) |

Indices use acquire/release ordering; the producer publishes with a
release store, the consumer reads with an acquire load.  `TXRDY` is
"tx has room"; `RXRDY` is "rx is not empty".  `push_frame` is
whole-frame-or-nothing, so the guest never sees half a frame.

### 4.4 Frames and sockets

`netTask` drains `txq` into a 4-byte header plus payload; it rejects a
declared length over 1024 and dispatches the command.  Socket state
lives in `Sock`: state, mode, a per-socket TCP backlog, bind port, UDP
peer, `WiFiUDP`, `WiFiClient` and an optional `WiFiServer`.

The handlers follow `nic.c`: OPEN/CLOSE/BIND/CONNECT/LISTEN/SEND/
STATUS/JOIN/GETCONF/SETCONF.  `poll_socket` drains a TCP backlog,
pushes arrived TCP or UDP data as `DATA`, accepts one client on a
listener, and reports `EVENT closed`/`reset`.  TCP `connect` is
blocking (Arduino has no non-blocking form here): the ACK is queued
before the attempt, and the network task spends up to five seconds on
it, so other sockets wait.

### 4.5 Startup and limits

`setup()` loads NVS credentials, builds the DI masks, sets the pin
directions, releases VI5, closes the sockets, disables core 1's
watchdog, starts the station or the setup AP, and creates the two
tasks.  `loop()` only sleeps; the tasks do the work.

Known limits, all matching or deliberately diverging from `nic.c`:

- TCP connect can block `netTask` for up to five seconds.
- UDP is client-shaped: `DATA` carries no peer, so a socket replies only
  to the peer named by `CONNECT` (TFTP and `nd` fit; arbitrary senders
  do not).
- `STATUS` reports rx 0 and tx as the unsent backlog.
- One connection per socket, and the listener closes after the first
  client; there is no `accept`.
- Frames over 1024 bytes are rejected.
- The card's receive ring is 8 KB; a full ring sheds UDP datagrams and
  shrinks the TCP window.

### 4.6 Build and flash

    make            # pio run
    make upload     # over the DOIT's own USB-UART
    make monitor    # 115200

Credentials: see `README.md`.
