# The ESP32 S-100 network card — firmware

The third implementation of the byte channel `sys/NET.md` specifies.
The guest driver is `sys/net.c`, the golden reference for this side is
the simulator's card model, `src/hwsim/d1/nic.c`, and this firmware is
the one that runs on the ESP32.

The board is the one in the card document added with the driver
(`git show 13bd5d78:docs/hw/esp32-nic.md`): an ESP32-DOIT DEVKIT V1 on
an S-100 buffered prototype board, 2:1 dividers on every 5V input, a
7406 pulling VI5, and nothing else.

## Pins

| S-100 | ESP32 | direction |
|---|---|---|
| DO0–DO7 | GPIO 32 33 25 26 27 14 12 13 | in, through dividers |
| DI0–DI7 | GPIO 21 22 23 18 19 5 16 17 | out, to the bus buffer |
| select | GPIO 34 | in, input-only |
| sOUT | GPIO 35 | in, input-only |
| sINP | GPIO 36 | in, input-only |
| A0 | GPIO 39 | in, input-only (high = NETSTAT, 0x61) |
| VI5 | GPIO 4 | out, 7406 open collector (high asserts) |

`select`, `sOUT` and `sINP` are treated as active high, as the card
document's poll loop reads them (`SEL_ACTIVE`, `SOUT_ACTIVE`,
`SINP_ACTIVE`, `A0_STAT_HIGH` in `src/main.cpp`).  A board that buffers
an inverted line needs the matching constant flipped.

GPIO 5, 12–15 are strapping pins at reset.  DI5 is GPIO 5 and DI6 is
GPIO 12; the bus buffer must not drive them at reset.  The rest of the
DI pins are ordinary outputs.

## Build and flash

    cp include/secrets.example.h include/secrets.h   # wifi, edit it
    make                 # pio run
    make upload          # over the DOIT's own USB-UART
    make monitor         # 115200

`secrets.h` is ignored by git; `-DWIFI_SSID=... -DWIFI_PASS=...` in
`platformio.ini` works too.  Without credentials, `JOIN` answers
`ACK timeout` and `RDY` never sets.

## What it is doing

Two tasks, two cores:

- **busTask**, core 1, highest priority, `IRAM_ATTR`, never yields.  It
  polls the bus, keeps DI driven with the byte a read would return (the
  rx head for NETDATA, the status byte for NETSTAT), consumes the head
  on a read strobe's leading edge, samples DO on a write strobe, and
  follows the rx FIFO with VI5.  Core 1's task watchdog is turned off
  because this loop deliberately starves its idle task.
- **netTask**, core 0, 100 Hz like `nic.c`'s poll.  It assembles frames
  from the tx FIFO, runs the socket state machine against lwIP, and
  pushes `ACK`/`DATA`/`EVENT` frames into the 8 KB rx FIFO.  Wi-Fi and
  the TCP/IP stack share this core.

The FIFOs are single-producer/single-consumer rings with acquire/release
atomics.  The interrupt is asserted by the producer after a frame is
queued and released by the consumer after the last byte is drained;
both sides do that under one spinlock, so the line cannot be left low
with data waiting.

## Deliberate differences and limits

- **TCP connect blocks.**  Arduino's `WiFiClient::connect()` has no
  non-blocking form here, so `CONNECT` ACKs immediately (as the contract
  requires) and then the network task spends up to five seconds
  establishing it.  The guest sees the ACK at once; other sockets wait.
- **UDP is client-shaped.**  `DATA` carries no peer address, so a UDP
  socket can reply to the peer named by `CONNECT` but cannot serve
  arbitrary senders.  That covers TFTP and `nd`; rlogin/telnet is TCP.
- **`STATUS` reports rx 0 and tx as the unsent backlog**, exactly as
  `nic.c` does: received data is pushed as `DATA`, not left in a
  per-socket receive buffer.
- A frame whose length exceeds 1024 is discarded with `ACK bad-state`
  rather than overrunning the assembly buffer (`nic.c` trusts it).
- The bus loop costs one byte per strobe; there is no latch.  If the
  timing ever bites, the card document's decision list already names
  the 74LS374 answer.

The guest side can be brought up under the simulator first: `d1` models
this same register face with `nic.c`, and `usersim` mounts it on
`/dev/net`.
