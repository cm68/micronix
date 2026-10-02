# MON5.0 and the external boot ROM

The switch-0 boot path has changed. `mon500` no longer carries an IDE
driver; instead it copies a 4K selector ROM down from the top of the address
space and runs it. This file describes that ROM, the monitor that uses it,
and where the two live. `docs/BOOT.md` remains the map of the other boot paths.

The monitor ROMs are `src/micronix/stand/roms/mon375.s`, `mon447.s` and
`mon500.s`; only `mon500` knows about the selector. The selector itself is
`src/micronix/stand/roms/multIO.s`, assembled to a 4096-byte `multIO.bin`
(the `multIO.bin` rule is `cp` the `.cim` then `truncate -s 4096` — there is
no on-board data below the code to skip, so no `dd`).

---

## 1. The two pieces

- **MON5.0** (`mon500.s`) — the 2K monitor. Its supervisor and trap half is
  unchanged. Its *boot* half lost the IDE (`ideboot`) and NCR 5380 SCSI
  (`ncrboot`) drivers; switch setting `0x00` now runs `copyrom`, which brings
  the external ROM down and enters it. The HD-DMA (`nuboot`, `0x08`) and
  DJ-DMA floppy (`djdma`, `0x10`) paths are still built in.

- **The external boot ROM** (`multIO.bin`, the "selector") — a 4K program
  org'd at `0xf000`, resident at physical `0xff0000`. It arms the IDE board
  and the NCR 5380 SCSI at once and lets them race: the first to answer ready
  wins, and its sector 0 is read into task 1's `0x100` and entered.

The external ROM is meant to plug into a **MultIO** board — or any board that
decodes the full IEEE 696 address bus — at `0xff0000`, the top 4K of the 16M
space.

---

## 2. The switch register (new decode)

The monitor reads the configuration switch at `0x402` and masks `0f8h`, the
same as before. A switch that is on reads as a zero bit, so `0` is
all-switches-on.

```asm
        cp  0                   ; all S1-S5 on
        jp  z,copyrom           ;  -- the selector
        cp  08h                 ; switch 5 off
        jp  z,nuboot            ;  -- HD-DMA hard disk
        cp  10h                 ; switch 4 off
        jr  z,djdma             ;  -- DJ-DMA floppy
```

The `0x18` (NCR SCSI) row is gone from the monitor — SCSI moved into the
selector. Bit 2 (`0x04`) is still the skip-the-monitor bit tested at `check`.

| `sw & 0xf8h` | `mon375.s` / `mon447.s` | `mon500.s` |
|---|---|---|
| `0x00` | `boothd` — HDCA | `copyrom` — the selector |
| `0x08` | `nuboot` — DMA hard disk | `nuboot` — DMA hard disk |
| `0x10` | `djdma` — DJ-DMA floppy | `djdma` — DJ-DMA floppy |
| `0x18` | — | — (SCSI lives in the selector) |

---

## 3. Where the ROM lives, and how it is reached

The task register's upper four bits gate **straight to S-100 A20–A23**. The
map registers (eight bits) drive A12–A19, and the low twelve address bits are
the page offset, so a physical address is

```
(A20..A23) bank  |  (A12..A19) page  |  (A0..A11) offset
```

The map alone reaches 1M; the bank nibble reaches the other 15M. The selector
sits at `0xff0000`, which is **bank 15, page `0xf0`** — a single 4K page, one
map entry, one bank. Writing the task register with `0xf0` selects bank 15
and keeps task 0, so the ROM's page is reachable through any segment whose map
entry is `0xf0`.

The reason this has to be the top 4K: the ROM is addressed by the same 24
lines a RAM board is, so the board it plugs into has to decode the full
IEEE 696 bus. A MultIO board does; a board that only decodes 16 or 20 lines
cannot put a device at `0xff0000`.

---

## 4. The copy

`copyrom` in `mon500.s` brings the selector down to `0xf000`. Two segments of
task 0's map are set once:

- segment 1 → page `0xf0` (the source, in bank 15),
- segment 2 → page `0x0f` (the destination, physical `0xf000` in bank 0).

Then sixteen 256-byte passes run through the **bounce buffer**, a 256-byte
scratch area at `0x032` in on-board RAM:

```
for off in 0, 256, 512, ..., 3840:
    bank in  (task = f0)   read 0x1000+off -> bounce   (this is 0xff0000+off)
    bank out (task = 00)   write bounce -> 0x2000+off  (this is 0xf000+off)
```

The bounce buffer is what makes this work. Source and destination are in
different 1M banks, so they are never visible at once; the buffer is below
`0x1000`, in the CPU board's own RAM, off the S-100 bus, so it stays put
across the bank flip.

There is one hazard that forces the loop out of ROM and into RAM. Writing the
task register — even a bank-only write — also ends reset and swaps the boot
ROM's lower half out (`setrom` in `mpz80.c`; the monitor's own "when the task
register is written into, the lower half of the prom goes away"). So the
bank-switching loop cannot run from ROM: `copyrom` copies an 83-byte routine
(the `ramcode` block, assembled with `.phase 132h`) into on-board RAM at
`0x132` and runs it there. On-board RAM survives both `setrom` and the bank
flip.

After the last pass, the loop does `jp 0f000h` — task 0's top segment, which
is the selector just copied.

---

## 5. The race

The selector's entry (org `0xf000`) arms both controllers, then polls them in
one loop:

```asm
        call idesetup        ; reset the IDE drive
        call scsisetup       ; reset the SCSI bus, select target 0
race:   call rdstat          ; IDE status -> DRDY?
        and  40h
        jr   nz,idewin
        in   a,(scsistat)    ; SCSI bus status -> BSY?
        and  scsi_bsy
        jr   nz,scsiwin
        ...                  ; bounded poll, then "no boot device"
```

`idesetup`/`scsisetup` return after arming; the poll loop is the race.
Whichever answers first wins:

- **IDE** — load the seven-byte task file, wait DRQ, read 512 bytes into the
  segment-1 window at `0x1100` (task 1's `0x100`), switch to task 1, jump.
- **SCSI** — send a ten-byte `READ(10)`, handshake 512 bytes in one REQ/ACK
  at a time into the same window, switch to task 1, jump.

The console is uart 0 (Wunderbus group 1), and the selector is chatty:
`probing ide+scsi`, then `ide`, `scsi`, or `no boot device`.

The task switch at the end is self-contained (the monitor's `check`/`nutask`
are not reachable after `setrom`): `switch` writes the usual `ld a,task /
ld (task),a / nops / jp` stub into `gobuff` (`0x1b0`) and runs it.

---

## 6. The boot flow, end to end

```
reset -> mon500 (rom0) -> switch decode -> copyrom
      -> copy selector 0xff0000 -> 0xf000 (via the bounce buffer)
      -> jp 0xf000 (task 0)
      -> selector: race IDE vs SCSI
      -> read sector 0 into task 1's 0x100
      -> task switch, jp 0x100 (the second-level loader)
```

---

## 7. Building, and the simulator's side

`make` in `src/hwsim/d1` recurses into `../../micronix/stand/roms`, which
builds the four `.bin`/`.sym` pairs. The simulator serves the selector as a
read-only `boot_rom` at `0xff0000` (`src/hwsim/s100.c`), loaded from
`multIO.bin` (`src/hwsim/hwsim.c`); `-M <file>` overrides the name, and the
default is resolved
relative to the tree. `-b ../../micronix/stand/roms/mon500.bin` selects the monitor that knows the
selector; `-c 0x04` is the all-switches-on + skip-monitor setting that reaches
`copyrom`.

The IDE and SCSI drivers answer ready (DRDY/BSY) only for a *formatted* drive,
so a bay with an empty image falls through the race instead of answering and
then hanging.
