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

    make                 # pio run
    make upload          # over the DOIT's own USB-UART
    make monitor         # 115200

## Provisioning

The network card comes up with no credentials and raises its own setup
access point; give it an SSID and password either way, and it stores
them in **NVS** (`Preferences`, namespace `nic`), so they survive the
power cycle and a flash of new firmware.

**1. Over its access point.**  Join the open AP named
`micronix-nic-xxxx` (the last two MAC bytes), then open
`http://192.168.4.1/` - any URL lands there through the captive-portal
DNS.  The page shows the station's state and takes the SSID and
password; *Save and restart* writes NVS and reboots.  Set
`NIC_AP_PASS` to put a WPA2 key on the setup AP instead of leaving it
open.

**2. Over the USB serial console** (115200, on the DOIT's own UART),
after the WiFiDongle's editor: send any character and it scans and lists
the networks, then prompts

    ssid: <current value, editable>
    password: <current value, editable>

with backspace/delete editing and Enter to accept.  It writes the pair
to NVS (no restart if nothing changed) and reboots.  This is the way in
when the setup access point is not convenient, and the fallback when a
provisioned network stops answering; the editor runs on the card's
network task, so the bus side keeps running while it waits for you.

After either path the station runs under **DHCP**: address, mask,
gateway and DNS all come from the network.  It registers the hostname
and mDNS name `micronix-nic-xxxx.local`, and `JOIN` answers with the
address; `RDY` follows the link.  If a provisioned network does not
answer within 20 seconds, the setup AP comes back up (in AP+STA mode)
so a wrong password is recoverable without erasing NVS.  `pio run -t
erase` clears the whole chip, NVS included.

The compile-time `secrets.h` / `-DWIFI_SSID=...` pair is only a bench
fallback, used when NVS holds nothing.

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
