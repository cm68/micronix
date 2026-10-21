# The ESP32 S-100 network card

Logical and physical layout for the card in `sys/NET.md`, built on the
s100computers.com **Buffered Prototyping Board** (V3.0a artwork) with a
**DOIT ESP32 DevKit V1** (30-pin).  Two I/O ports, one open-collector
interrupt, and a small amount of glue; the board's existing 74LS245s and
74LS682 do the bus buffering and the port decode.

The rendered layout with the scaled floor plan and block diagram is
`esp32-nic.html` beside this file.  The placement-and-wiring sheet is
`esp32-nic-wiring.svg` (and `.png`).

## 1. The contract

    NETDATA  0x60   read: next RX FIFO byte      write: one TX FIFO byte
    NETSTAT  0x61   read: RXRDY 0x01
                          TXRDY 0x02
                          RDY   0x04              write: ignored

The card pulls `/VI5` low while the RX FIFO holds a byte and releases it
when the guest drains the FIFO.  VI5 is S-100 edge pin 9; it is also
brought out on the proto board at P23.8.  The line is level-asserted and
open collector: the 7406 sinks it, a 4.7 kΩ pull-up restores it.

The V2 board has the same P40-P48 headers and the same 74LS682, but its
74LS682 Q jumpers are three-pin GND/Q/VCC shunts.  The V3 board uses
two-pin shunts to GND and relies on the 682's internal Q pull-ups.  Both
settings are given below.

## 2. Logical layout

The proto board buffers the S-100 address, data and control lines in
74LS245s and decodes the port in a 74LS682.  The card adds:

- a receive path: P54 (DO0b-DO7b) through a 74LS373 write latch, then
  P44.9 (sOUTb), P44.7 (sINPb), P40.5 (A0b) and P47.9 (SEL*) through two
  74LVC245s into the ESP32;
- a data-in path: ESP32 DI0b-DI7b straight into P50, then through the
  board's U107 to the S-100 DI lines;
- an interrupt driver: GPIO12 through a 7406 to the VI5 line.

### Signal schedule

| Signal | Board pad / header | Direction | GPIO | Module pad (P7) |
|---|---|---|---|---|
| A0b | P40.5 | to card | 39 (VN) | B1 |
| SEL* (682 /P=Q) | P47.9 | to card, to U107/U109 /OE | 34 | D1 |
| sOUTb | P44.9 | to card | 35 | E1 |
| sINPb | P44.7 | to card | 36 (VP) | C1 |
| DO0b-DO7b | P54.1-P54.8 | to card | 32 33 25 26 27 14 13 4 | F1 G1 H1 I1 J1 K1 M1 K10 |
| DI0b-DI7b | P50.1-P50.8 | card to bus | 16 17 5 18 19 21 22 23 | J10 I10 H10 G10 F10 E10 B10 A10 |
| VI5* | P23.8 / J1.9 | card to bus | 12 | L1 |
| +5 V | P57/P59/... VCC rail | to card | VIN | O1 |
| +3.3 V | module 3V3 | to LVC245 | 3V3 | O10 |
| GND | P56/P58/... rail | - | GND | N1, N10 |

Module pads are PGA positions in the P7 socket: column 1 is the left
header, column 10 the right; rows A-O run from the antenna end toward the
USB end.  The module's own header order is EN, VN, VP, 34, 35, 32, 33,
25, 26, 27, 14, 12, 13, GND, VIN on the left and 23, 22, TX0, RX0, 21,
19, 18, 5, 17, 16, 4, 2, 15, GND, 3V3 on the right.  Verify against the
silkscreen before soldering; some online diagrams transpose VP and VN.

DO0b-DO7b do not go straight to U20: they enter the 74LS373 write latch
first, and the latch's Q outputs drive U20 A0-A7.  The latch is what
makes the 6 MHz timing possible.

### Write latch and 6 MHz

At 6 MHz the Z80's I/O write window is only about 250 ns, too tight to
trust a firmware poll of DO0b-DO7b.  Latch the byte in hardware:

- U23 74LS373: D0-D7 = P54.1-P54.8 (DO0b-DO7b); Q0-Q7 = U20 A0-A7;
  LE = sOUTb AND SEL*; /OE = GND; VCC = +5 V; GND = pin 10.
- 10 kΩ pull-downs on the D inputs (P54) so they are defined when U109
  is disabled.
- U24 74LS00 makes LE: gate 1 inverts SEL* to select; gate 2 NANDs sOUTb
  with select; gate 3 inverts that NAND to an AND.  Tie the unused gate's
  inputs to +5 V or GND.

The latch is transparent while sOUT is high and holds the byte when it
falls.  The ESP32 polls sOUTb for a rising edge with SEL* low and reads
the held byte through U20 at its own speed.  The 6 MHz strobe is 250 ns
(60 cycles at 240 MHz), so core 0 sees the edge; the data is stable
before and after it.

74LS373 DIP-20 pinout:

| Pin | Signal | Pin | Signal |
|---|---|---|---|
| 1 | /OE (GND) | 2 | Q0 |
| 3 | D0 | 4 | D1 |
| 5 | Q1 | 6 | Q2 |
| 7 | D2 | 8 | D3 |
| 9 | Q3 | 10 | GND |
| 11 | LE (from U24) | 12 | Q4 |
| 13 | D4 | 14 | D5 |
| 15 | Q5 | 16 | Q6 |
| 17 | D6 | 18 | D7 |
| 19 | Q7 | 20 | VCC (+5 V) |

D0-D7 = P54.1-8; Q0-Q7 = U20 A0-A7.

U24 74LS00 gate map:

| Gate | Pins | Connection |
|---|---|---|
| 1 | 1,2 -> 3 | SEL* (P47.9) -> select (NOT SEL*) |
| 2 | 4,5 -> 6 | sOUTb (P44.9), select -> n (NAND) |
| 3 | 9,10 -> 8 | n -> LE |
| - | 7, 14 | GND, +5 V |

The unused fourth gate (pins 12,13 -> 11) should have its inputs tied to
GND or +5 V.


### Board straps

| Buffer / net | Strap | Why |
|---|---|---|
| U101 DIR / OE | P40.1 -> VCC, P40.3 -> GND | enable A0b, A->B |
| U105 DIR / OE | P44.1 -> VCC, P44.3 -> GND | enable sOUTb, sINPb |
| U102, U103, U104 | P41.3 / P42.3 / P43.3 -> VCC | unused, disable outputs |
| U106 (spare 244) | P45.1, P45.3 -> VCC | unused, disable |
| U107 DI buffer | P46.1 -> GND, P46.3 -> SEL* | B->A, drive only our port |
| U109 DO buffer | P48.1 -> VCC, P48.3 -> SEL* | A->B, mirror only our port |
| U108 (spare 245) | P49.3 -> VCC | unused, disable |
| 682 Q jumpers | fit JP11, JP8, JP7, JP6, JP5, JP4; leave JP10, JP9 open | 0x60; fit = Q low, open = internal pull-up high (V3) |
| 682 A0 input | lift U110 pin 17, wire to GND | ignore A0 so 0x61 matches |
| SEL* pull-up | 4.7 kΩ P47.9 to +5 V | 682 output is open collector |

On a V2 board, shunt pins 1-2 for Q = 0 and pins 2-3 for Q = 1.

### The one mod

The 682 compares A7...A0, with A0 on P7 (pin 17).  To make it match both
0x60 and 0x61, lift U110 pin 17 from the A0 net and wire it to ground;
fit JP4 (Q7 = 0).  Without this, the card answers 0x60 only.  If you will
not cut a trace, a second 74LS682 or a 74LS688 dead-bugged on the proto
area can decode A7...A1 instead; the rest of the layout is unchanged.

The 682 sees only A0-A7, so the upper I/O byte is ignored, as on any
8-bit S-100 card.  If your system pages I/O with A8-A15 and another board
also answers 0x60 in a different page, add a second comparator on those
lines.

## 3. Level translation

The board's "buffered" pads are 74LS245 outputs running from a 5 V rail.
An LS high is **2.7 V minimum, about 3.4 V typical**, not 5.0 V.  The
ESP32's input-high threshold is 0.75 x 3.3 V = 2.48 V, and its absolute
maximum on a GPIO is 3.6 V.

A 2:1 divider from an LS high lands near 1.7 V, below the ESP32's
threshold.  The earlier note that a divider gives "about 2.5 V" assumed a
5.0 V source that these pads do not provide.  Do not use 10 kΩ/10 kΩ
dividers here.

Use two **74LVC245** bus transceivers powered at **3.3 V**: VCC on pin
20 from the module's 3V3, GND on pin 10.  The LVC inputs are 5 V
tolerant to 5.5 V, so both the LS pads and any HCT substitution are
safe; Vih is 2.0 V at VCC = 3.3 V, so an LS 2.7 V minimum has margin;
the outputs are 3.3 V and drive the ESP32 directly.  Use each '245 as a
one-way buffer: DIR (pin 1) tied to 3.3 V for A->B, /OE (pin 19) tied to
ground.  The A side is the board's buffered pads, the B side the ESP32.
U20 carries the latched Q0-Q7 on A0-A7; U21 carries sOUTb, sINPb, SEL*
and A0b on A0-A3.  The spare channels on U21 (A4-A7, pins 6-9) must
have their inputs tied to ground and their B outputs left open.  Two
packages cover the twelve inputs.

The pinout is the familiar '245: DIR 1, A0-A7 2-9, GND 10, B7-B0 11-18,
/OE 19, VCC 20.  Note that the B side runs backwards: B0 is pin 18 and
B7 is pin 11.

**Never put an LVC245 in one of the board's 5 V buffer sockets.**  These
two are new positions in P14, powered from 3.3 V; U101-U109 stay on the
5 V rail.

If you would rather use the through-hole part, two **CD4050BE** hex
buffers at VDD = 3.3 V are the alternate: inputs tolerate 18 V, Vih is
about 0.7 x VDD (2.3 V), outputs are 0-3.3 V, and they are slower.  The
signals are the same, but the pinout and the P14 sockets differ (DIP-16).

Do not use an LVC245 for VI5; it is push-pull, and the interrupt line
needs the open-collector 7406 (or a 74HCT06).  If the board can be
powered with the module removed and the 5 V bus live, 1 kΩ series
resistors on the twelve A inputs limit the current through the LVC input
protection with VCC = 0.

The other direction needs no translator.  The ESP32's 3.3 V high is above
the 74LS245's 2.0 V VIH, so DI0b-DI7b drive U107's B-side inputs
directly.  The 7406 is also a TTL-threshold part, so GPIO12 drives its
input directly.

Add 10 kΩ pull-downs on DO0b-DO7b: U109's B side follows the S-100 DO
lines, which float when no write is in progress.  GPIO5 is one of the DI
outputs (a strapping pin), so fit a 10 kΩ pull-up from module pad H10 to
3.3 V.  GPIO12 is the interrupt output, so fit a 10 kΩ pull-down from L1
to ground.  GPIO15 is unused.

## 4. Physical layout

The board is 254 x 127 mm (10 x 5 in) with the 100-pin edge tongue along
the bottom.  The top half is twelve 15 x 15 PGA wire-wrap sockets in two
rows (P4-P9 and P10-P14, P19); the port decode and the bus buffers sit
along the bottom.

| Part | Location | Center (x, y mm) | Mounting note |
|---|---|---|---|
| DOIT ESP32 DevKit V1 | P7, columns 1 and 10, rows A-O | 167.64, 42.55 | two 1x15 female headers; antenna toward y = 22.86 |
| U23 74LS373 latch | P14, rows B-E | 205.74, 69.2 | DIP-20 socket |
| U20 74LVC245 (data) | P14, rows G-J | 205.74, 81.9 | DIP-20 socket |
| U21 74LVC245 (control) | P14, rows K-N | 205.74, 92.1 | DIP-20 socket |
| U22 7406 | P10, rows G-J | 53.34, 81.9 | DIP-14 socket |
| U24 74LS00 | P10, rows B-E | 53.34, 69.2 | DIP-14 socket |
| DO pull-downs, 8x 10 kΩ | P14 / P54 | - | 9-pin bussed SIP or discrete |
| SEL* 4.7 kΩ, VI5 4.7 kΩ | P10 / P47 | - | to +5 V |
| GPIO5 pull-up, GPIO12 pull-down | P7 H10 / L1 | - | 10 kΩ, to 3.3 V / GND |

### Mechanic notes

- **Antenna.**  The module's PCB antenna is at its top end.  In the P7
  socket it overhangs the board edge by only about 4 mm; the rest of the
  keepout is over the P7 pads.  That is enough for bench work and
  marginal in a metal S-100 chassis.  In a metal box, mount the module on
  standoffs 10-15 mm further toward the top edge and feed it with a short
  2x15 ribbon, or use an ESP32-WROOM-32U with an external antenna on a
  bulkhead connector.  Do not put a metal panel or the card guide over
  the antenna.
- **USB.**  The connector is at the module's lower end, over the P13
  area.  Program the module before final assembly or leave a right-angle
  cable path; P13 is empty for exactly this reason.
- **Height.**  The module sits on 8.5 mm female headers.  Its lower end
  overhangs P13 by about 10 mm, clearing the empty socket.  Do not
  install a wire-wrap socket in P13.
- **Wiring.**  Use 30 AWG wire-wrap for the bus signals and keep the
  DO/DI runs short.  The SEL* run from P47/U110 to P14 is the longest at
  about 130 mm; it is a level, not a strobe, so it is not timing
  critical.  Twist the VI5 run with a ground return if the chassis is
  noisy.

### Wiring groups

| Group | From | To | Notes |
|---|---|---|---|
| A latch | P54.1-8 (DO0b-DO7b) | U23 D0-D7 (pins 3,4,7,8,13,14,17,18) | 10 kΩ pull-down at each D input |
| | U23 Q0-Q7 (pins 2,5,6,9,12,15,16,19) | U20 A0-A7 (pins 2-9) | |
| B receive | U20 B0-B7 (pins 18-11) | P7 F1 G1 H1 I1 J1 K1 M1 K10 | DIR=3V3, /OE=GND; GPIO 32 33 25 26 27 14 13 4 |
| C control | P44.9, P44.7, P47.9, P40.5 | U21 A0-A3 (pins 2-5) | DIR=3V3, /OE=GND |
| | U21 B0-B3 (pins 18-15) | P7 E1 C1 D1 B1 | GPIO 35 36 34 39 |
| | U21 A4-A7 (pins 6-9) | GND | spare channels; B4-B7 open |
| D data-in | P7 J10 I10 H10 G10 F10 E10 B10 A10 | P50.1-8 (DI0b-DI7b) | direct 3.3 V; GPIO 16 17 5 18 19 21 22 23 |
| E latch gate | P47.9 (SEL*), P44.9 (sOUTb) | U24 74LS00: pins 1,2; pin 3 -> pin 5; pin 4 = sOUTb; pin 8 | pin 8 -> U23 LE (pin 11); pins 7/14 = GND/+5 V |
| F buffer gates | P47.9 (SEL*) | P46.3, P48.3 | 4.7 kΩ pull-up to +5 V at P47.9 |
| G interrupt | P7 L1 (GPIO12) | U22 pin 1 | 10 kΩ pull-down; U22 pin 2 to P23.8; 4.7 kΩ to +5 V |
| H power | VCC rail | P7 O1, U22 VCC, U23/U24 VCC, pull-ups | +5 V |
| | P7 O10 (3V3) | U20/U21 pin 20 | +3.3 V; 10 uF + 100 nF |

### LVC245 pin map

| '245 pin | U20 (data) | U21 (control) |
|---|---|---|
| 1 DIR | 3V3 | 3V3 |
| 2 A0 | U23 Q0 (pin 2) | P44.9 sOUTb |
| 3 A1 | U23 Q1 (pin 5) | P44.7 sINPb |
| 4 A2 | U23 Q2 (pin 6) | P47.9 SEL* |
| 5 A3 | U23 Q3 (pin 9) | P40.5 A0b |
| 6-9 A4-A7 | U23 Q4-Q7 (pins 12,15,16,19) | GND |
| 10 GND | GND | GND |
| 11 B7 | GPIO4 DO7 | open |
| 12 B6 | GPIO13 DO6 | open |
| 13 B5 | GPIO14 DO5 | open |
| 14 B4 | GPIO27 DO4 | open |
| 15 B3 | GPIO26 DO3 | GPIO39 A0 |
| 16 B2 | GPIO25 DO2 | GPIO34 SEL |
| 17 B1 | GPIO33 DO1 | GPIO36 sINP |
| 18 B0 | GPIO32 DO0 | GPIO35 sOUT |
| 19 /OE | GND | GND |
| 20 VCC | 3V3 | 3V3 |

### 74LS373 latch map

| D | From | Q | To |
|---|---|---|---|
| D0 (3) | P54.1 | Q0 (2) | U20 A0 (2) |
| D1 (4) | P54.2 | Q1 (5) | U20 A1 (3) |
| D2 (7) | P54.3 | Q2 (6) | U20 A2 (4) |
| D3 (8) | P54.4 | Q3 (9) | U20 A3 (5) |
| D4 (13) | P54.5 | Q4 (12) | U20 A4 (6) |
| D5 (14) | P54.6 | Q5 (15) | U20 A5 (7) |
| D6 (17) | P54.7 | Q6 (16) | U20 A6 (8) |
| D7 (18) | P54.8 | Q7 (19) | U20 A7 (9) |

## 5. Power

    S-100 +8 V (pins 1, 51)
       -> P70 1-2 -> U1 LM7805 -> +5 V (VCC rail)
                                    -> 74LS buffers, 7406, pull-ups
                                    -> DOIT VIN (O1) -> AMS1117-3.3 -> 3.3 V
                                                                         -> ESP32
                                                                         -> LVC245 VCC

Fit U1 and jumper P70 1-2; fit K12 1-2 for U1's ground.  Leave U2/U3 and
their input jumpers unpopulated; the card uses no +/-12 V.

Budget about 360 mA at 5 V: ~120 mA of LS logic, ~240 mA ESP32 peak
during WiFi transmit.  The 7805 drops about 3 V at that current, so fit a
small TO-220 heatsink.  +8 V is unregulated and can sit near 7 V at the
low end, which is marginal for a 7805 at full WiFi current; if your +8 V
rail is weak, feed the module's VIN from a separate 5 V supply for bench
work.

Decoupling: 470 uF/10 V at the module VIN, 100 nF at VIN and 3V3, 100 nF
at each LVC245, 100 nF at the 74LS373 and 74LS00, and 100 nF at the
7406.  The '245 pinout is pin 20 VCC (3.3 V), pin 10 GND, pin 1 DIR,
pin 19 /OE.  Do not power them from +5 V.  The latch and the gate run
from +5 V.

### Bill of materials

| Qty | Part | Use |
|---|---|---|
| 1x | DOIT ESP32 DevKit V1, 30-pin | the coprocessor |
| 2x | 74LVC245, DIP-20 | 5 V to 3.3 V, U20/U21 |
| 1x | 74LS373, DIP-20 | DO write latch, U23 |
| 1x | 74LS00, DIP-14 | latch LE gate, U24 |
| 1x | 7406, DIP-14 | open-collector VI5 driver, U22 |
| 2x | 1x15 female header, 2.54 mm | P7 columns 1 and 10 |
| 3x | DIP-20 socket | U20, U21, U23 |
| 2x | DIP-14 socket | U22, U24 |
| 8x | 10 kΩ | DO0b-DO7b pull-downs |
| 2x | 10 kΩ | GPIO5 pull-up, GPIO12 pull-down |
| 2x | 4.7 kΩ | SEL* and VI5 pull-ups |
| 1x | 470 uF / 10 V | module VIN bulk |
| 1x | 10 uF | 3V3 at the LVC245s |
| 6x | 100 nF | VIN, 3V3, each LVC245, U23, U24, 7406 |
| - | 30 AWG wire-wrap, SIP headers | straps and signal runs |
| 2x | CD4050BE, DIP-16 (alternate) | through-hole level shifter if no LVC245s |

## 6. Bring-up

1. Build the board first, module last.  Strap the unused buffers, fit the
   682 jumpers, make the A0 cut, and add the pull-ups before the module
   is plugged in.
2. Bench power.  Use an S-100 extender and feed +8 V and ground only.
   Check +5 V at the VCC rail and 3.3 V at P7 O10 before fitting the
   module.  The module's USB is the easy way to check that it boots.
3. Static bus test.  With a logic analyzer or a slow Z80, read 0x61 after
   reset: RDY (0x04) should be set once the firmware has joined, and
   TXRDY (0x02) always.  Check SEL* pulses only at 0x60 and 0x61, and
   not at 0x62.
4. Keep VI5 low at first.  The current guest driver (`sys/net.c`) polls
   RXRDY and does not claim the VI5 line.  Leave GPIO12 low so the 7406
   holds VI5 deasserted; bring the card up on the polled driver.  Enable
   the interrupt only after the guest driver attaches VI5
   (`sys/INTR.md`); an unclaimed level-asserted line is not something to
   discover on a live bus.
5. Byte loop.  Write a byte to 0x60 and read it back through the
   firmware's loopback, then exercise the FIFO status in 0x61.  Verify
   U23: LE pulses only when SEL* is low, the D inputs are pulled down
   when U109 is disabled, and Q0-Q7 hold the byte after sOUT falls.
   Check the DI setup before sINP.
6. Interrupt.  Once the driver claims the line, confirm VI5 follows the
   RX FIFO: low while a frame is waiting, high after the last byte is
   read.
7. Network.  Join, open a UDP socket, and echo against a host.  Then run
   `rcp -r` against the tree; that is the same guest binary the
   simulator runs, so a failure is in the bus interface, not the framing.

Test points: TP1 +5 V (VCC rail / P57.1), TP2 +3.3 V (P7 O10), TP3 SEL*
(P47.9), TP4 VI5 (P23.8), TP5 DI0 (P50.1), TP6 DO0 (P54.1).

## 7. Timing

The Z80 gives a peripheral roughly 750 ns to answer an I/O cycle at
2 MHz, 375 ns at 4 MHz, and about 250 ns at 6 MHz.  The read path is
pre-driven: the ESP32 keeps DI0b-DI7b set to
`A0b ? status : rx_peek()`, and U107's `/OE = SEL*` puts that byte on the
S-100 DI lines as soon as the port is addressed.  The only delay in the
read is the LS245 enable, about 15 ns; the 682 decode and the LVC245 add
about 30 ns.  That is well inside the 6 MHz window.

The write path no longer races the CPU.  U23 latches DO0b-DO7b on
sOUTb, gated by SEL*, and holds the byte after sOUT falls; the ESP32
reads the latch through U20 at its own speed.  The only thing the
firmware must catch is the sOUT edge: at 6 MHz it is about 250 ns wide,
60 cycles at 240 MHz, so core 0's poll loop sees it.  The 74LS373's
setup time is about 20 ns and the U24 gate adds about 10 ns, both
comfortable inside the strobe.

The ESP32 must consume one latched byte before the next write to our
port.  The guest's own flow control and the 240 MHz loop make that easy
(one byte per 667 ns at 6 MHz); if a future bus master writes the port
back to back faster than that, the latch is overwritten and the byte is
lost.

If the CPU ever needs more time than the card can give, the S-100 RDY
line (pin 72, on the proto board at P26.3) can be pulled low through a
second 7406 until the byte is ready; that is the clean way to add wait
states, but it is a second handshake and should be added only if the
latch is not enough.

## 8. Firmware map

Core 0 owns the bus and never calls into lwIP; core 1 owns the stack.  The
two exchange bytes through single-producer/single-consumer ring buffers.

```c
/* bus side, core 0 */
#define SEL   34   /* active low, from the 682 */
#define A0    39
#define SOUT  35
#define SINP  36
#define INT   12   /* drives the 7406; low = VI5 released */
#define DO(n) do_pin[n]   /* 32 33 25 26 27 14 13 4  */
#define DI(n) di_pin[n]   /* 16 17 5 18 19 21 22 23  */

static int prev_sout;

for (;;) {
    int sel = !gpio_get(SEL);
    int a0  =  gpio_get(A0);
    int sout = gpio_get(SOUT);

    /* read side: pre-drive the byte the CPU will take */
    if (sel)
        gpio_put8(DI, a0 ? netstat() : rx_peek());

    /* write side: U23 has latched DO; take it when sOUT falls */
    if (sel && !sout && prev_sout)
        tx_put(gpio_get8(DO));   /* U20 B -> U23 Q */
    prev_sout = sout;

    /* a NETDATA read pops the FIFO */
    if (sel && !a0 && gpio_get(SINP))
        rx_pop();

    /* level-asserted interrupt, only if the guest has claimed it */
    gpio_put(INT, rx_count() != 0 && irq_enabled);
}
```

The loop is illustrative.  Read the GPIO registers in one batch where the
part allows it and keep the core free of delays.  The simulator's
`hwsim/d1/nic.c` is the golden reference for the frame codec and socket
states; only the byte channel is replaced by this loop.

## 9. References

- S100Computers, Buffered Prototype Board:
  http://s100computers.com/My%20System%20Pages/Prototype%20Board/Prototype%20Board.htm
  — the V3.0a KiCad/Gerber archives are linked there.  The placement
  table above is taken from that archive's netlist and board file.
- 74LS682 port decoding (deramp MFE archive):
  https://deramp.com/downloads/mfe_archive/010-S100%20Computers%20and%20Boards/03-S100%20Computers%20-%20N8VEM/10-S100%20Computers%20S-100%20Boards/S-100%20Prototype%20Board/PU-74LS682.htm
- DOIT ESP32 DevKit V1 board model:
  https://github.com/wokwi/wokwi-boards/blob/main/boards/esp32-devkit-v1/board.json
  and https://www.schematik.io/parts/board-esp32-devkit-v1
- SN74LVC245A datasheet (the level shifter):
  https://www.ti.com/product/SN74LVC245A
- CD4049UB/CD4050B datasheet (the through-hole alternate):
  https://www.ti.com/lit/ds/symlink/cd4049ub.pdf
- Espressif ESP32 hardware design guidelines (antenna keepout):
  https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/esp-hardware-design-guidelines-en-master-esp32.pdf
- Project files: `src/micronix/sys/NET.md` (the contract),
  `src/hwsim/d1/nic.c` (the reference implementation),
  `src/micronix/sys/net.c` (the guest driver), and
  `src/micronix/sys/INTR.md` (the VI-line rules).
