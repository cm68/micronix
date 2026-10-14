# The ESP32 S-100 network card

How the network card (sys/NET.md) becomes hardware: an ESP32-DOIT module
on an S-100 buffered prototype board, with nothing but jelly-bean glue
(resistors, one optional 74HCT) between the two.  The ESP32 does all of
tcp/ip (lwIP) and the frame protocol; this document is the bus interface
that lets the Z80 hand it bytes.

## 1. The contract (NET.md section 1)

Two ports on the S-100 bus:

    0x60 NETDATA   read:  the next byte from the RX FIFO
                   write: a byte into the TX FIFO
    0x61 NETSTAT   read:  RXRDY (0x01) | TXRDY (0x02) | RDY (0x04)

Interrupt-driven - the card asserts VI5 (open-collector) whenever RXRDY
is set.  The framing, the socket states and the OPEN/CONNECT/SEND/STATUS
command set (NET.md sections 2-5) are all firmware; this is the board
that gets the bytes across.

## 2. The board

The buffered prototype board is why the glue is small.  It pre-wires the
address, data and control lines to bus buffers, and its 74LS682s plus the
port jumpers do the I/O address decode: set them to this card's base, and
the board asserts a buffered *select* whenever the CPU addresses these
ports.  U108 (a 74LS245) buffers the 8-bit data in both directions.

Everything below hangs off the buffered pads the board provides; the
board has already absorbed the fan-out and the direction control.

## 3. The shape of it

        S-100 bus
           |
     [proto board: 74LS245 data, 74LS682 select]
           |
     DO0-7  --[2:1 divider]--> ESP32   (write: CPU -> card)
     select --[2:1 divider]--> ESP32
     sOUT   --[2:1 divider]--> ESP32
     sINP   --[2:1 divider]--> ESP32
     A0     --[2:1 divider]--> ESP32   (0x60 vs 0x61)
     ESP32 ------------------> DI0-7   (read: card -> CPU)
     ESP32 --[7406 open-collector]--> VI5   (interrupt: RXRDY)
           |
      [lwIP: wifi, the socket face]

The ESP32 owns everything behind the divider bank: the two FIFOs, the
frame codec, and the tcp/ip stack.

## 4. Level translation

The one real problem, and it is asymmetric.

**Bus to ESP32 (5V -> 3.3V).**  The ESP32's GPIOs are not 5V-tolerant,
so every 5V input is divided 2:1 (two 10K, or 2.2K over 3.3K for more
margin) to land between the ESP32's thresholds - about 2.5V, under the
3.3V rail and over Vih.  Eleven dividers: DO0-7, select, sOUT, sINP.

**ESP32 to bus (3.3V -> 5V).**  No translation needed: a 3.3V high is
already above the 74LS Vih of 2.0V, so the ESP32 drives the board's
74LS245 directly and the board drives the bus at 5V.  A 74HCT244 is
optional margin if the run is long or the board's buffer is marginal.

## 5. The bus handshake

A Z80 I/O cycle is about a microsecond.  One ESP32 core runs a tight
poll loop:

    if (select) {
        if (sOUT) {                          # a write
            byte = read(DO0-7)
            if (A0 == 0) tx_fifo_put(byte)   # NETDATA
            else         /* NETSTAT is read-only */
        } else if (sINP) {                   # a read
            if (A0 == 0) byte = rx_fifo_get()  # NETDATA
            else         byte = status         # NETSTAT
            drive(DI0-7, byte)
        }
    }

The read path is the critical one - the byte has to be on DI0-7 inside
the sINP window - but a handful of GPIO reads and a write at 240 MHz are
well inside it at a 2-4 MHz Z80.

The interrupt is separate and upward: whenever the RX FIFO goes non-empty
the ESP32 asserts VI5, and it holds the line until the guest has drained
the FIFO (each read of NETDATA drops the line when the FIFO empties).  The
guest therefore sleeps on the line instead of busy-waiting on RXRDY; the
ESP32 still polls sOUT to catch the CPU's writes, because those are
synchronous with the CPU and need no line.

## 6. Pinout

The DOIT DEVKIT V1 exposes 38 pins; GPIO 1/3 are the USB-UART and GPIO
6-11 are the flash, so roughly 28 are free.  The card uses 20:

    DO0-7  -> GPIO 32 33 25 26 27 14 12 13    (in, through dividers)
    DI0-7  -> GPIO 21 22 23 18 19  5 16 17    (out)
    select -> GPIO 34
    sOUT   -> GPIO 35
    sINP   -> GPIO 36
    A0     -> GPIO 39
    VI5    <- GPIO 4  (out, open-collector via a 7406)

GPIO 34-39 are input-only, which suits the four control lines.  Power
comes from the board's 5V rail into the DOIT's VIN (its onboard
AMS1117-3.3 makes the 3.3V); the board carries the 5V across the patch
area.

## 7. Firmware

Three layers, all in the ESP32:

1. the byte-channel driver - the poll loop above, plus the two FIFOs;
2. the frame codec - NET.md sections 2-5, exactly what the simulator's
   nic.c models;
3. lwIP - JOIN/CONNECT/SEND against the wifi.

The poll loop owns one core so wifi processing never delays the bus
response; the other core runs the network stack.  nic.c is the golden
reference, so the guest driver (sys/net.c) and the rcp client run
against the card unchanged.

## 8. Bill of materials

    1x  ESP32-DOIT DEVKIT V1        the coprocessor
    22x resistors (11x 2:1 divider) 5V -> 3.3V on the inputs
    1x  7406 (hex inverter)          open-collector driver for the VI line
    1x  74HCT244 (optional)         3.3V -> 5V read-data margin
    1x  buffered prototype board    bus buffer + address decode

## 9. Decisions left

- **Polling vs a latch.**  The read path is firmware-timed.  A 74LS374
  (jelly bean) would latch the write and loosen the poll loop at the cost
  of a chip; hold it in reserve if the timing ever bites.
- **Address span.**  The board's comparator must span both 0x60 and 0x61
  (match the seven high bits, let A0 pick the port); confirm the jumper
  arrangement, or use two comparator settings.
- **Interrupt line.**  VI5 is the spare DISK3 line (the guest driver's
  `INT_NIC`); the ESP32 pulls it through a 7406, and the guest's
  interrupt handler drains the RX FIFO to drop it.  Level-asserted, the
  same convention the kernel's 8259 already uses.
- **Programming.**  The DOIT programs over its own USB-UART, so no
  in-circuit programmer lives on the card.

References: the S-100 [Buffered Prototype Board](http://s100computers.com/My%20System%20Pages/Prototype%20Board/Prototype%20Board.htm)
and [Mini Buffered Prototype Board](http://s100computers.com/My%20System%20Pages/Mini%20Buffered%20Prototype%20Board/Mini%20Buffered%20Prototype%20Board.htm)
at [S100Computers](http://s100computers.com/).
