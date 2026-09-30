# How the Decision One boots

Every way this machine gets from a reset to a running kernel, and where each
piece of code lives.

This is the map. `DJBOOT.md` is the deep dive on one of the five paths — the
DJ-DMA floppy — and is not repeated here.

Sources are named as they are used. The three monitor ROMs are
`src/micronix/stand/roms/mon375.s`, `mon447.s` and `mon500.s`. Each assembles to a
4096-byte `.cim`, of which the `Makefile` keeps the low half — `dd bs=1024
skip=2 count=2` — giving a 2048-byte `monNNN.bin`. `mon447` is what the
simulator loads by default (`mpz80_setup()`), and `m16boot` names it
explicitly.

---

## 0. The one structural fact

**While the ROM is running, every address below `0x1000` is the CPU board's
own memory — its static RAM and its PROM — and not main memory.**

Main memory is reachable from the ROM only at or above `0x1000`, through the
memory map. The ROM opens a window instead: it points task 0's segment 1 at
physical page 0, which makes an address in the `0x1000`–`0x1FFF` range in the
supervisor the same byte as the corresponding address in the first 4K of main
memory. `mon500.s`'s `ideboot` states this in as many words:

> The rom runs as the supervisor, and while it does, every address below
> 1000h is the cpu board's own ram and prom - so a store to 100h lands in the
> board's 1K static ram, not in the memory task 1 will see. Main memory is
> reachable only at or above 1000h, through the map. So the transfer runs with
> task 0's segment 1 pointed at physical page 0, which makes 1100h in the
> supervisor the same byte as 100h in task 1.

This explains the monitors' equates, which otherwise look arbitrary:

| equate | value | what it really is |
|---|---|---|
| `chan` | `1080h` | the DJ-DMA channel program block, at **physical `0x0080`** |
| `djstat` | `104ah` | the DJ-DMA done/status byte, at **physical `0x004a`** |
| `iopb` | `1050h` | the HDC-DMA channel address pointer, physical `0x0050` |
| `bootad` | `1100h` | the HDC-DMA bootstrap landing site, physical `0x0100` |

It is also why every loader in `stand/boot` links at `0x100`: by the time the
second level runs, the window is closed and `0x100` *is* `0x100`. The one
exception is `djboot1.s`, which links at `0x80`, because the DJ-DMA controller
leaves its level 0 there and level 1 is what follows it — see §4.

---

## 1. The switch register picks the path

The ROM reads the configuration switch at port `0x402` (`SWT` in
`mpz80.c`) and masks it with `0f8h`, "Ignore irrelevent bits". **The switch is
negated: a switch that is on reads as a zero bit.** So the value `0` means
every switch is on, which is why the first comparison is `cp 0`.

```asm
tstsw:  ld  a,(switch)
        and 0f8h
        ld  d,a                 ; d&e become the new task's PC
        ld  e,0H
        cp  0                   ; all switches on
        jp  z,boothd            ;  -- mon375/mon447
        cp  08h                 ; switch 5 off, others on
        jp  z,nuboot            ;  -- the DMA hard disk
        cp  10h                 ; switch 4 off, others on
        jr  z,djdma             ;  -- the DJ-DMA floppy
        cp  18h                 ; switches 5 and 4 off (mon500 only)
        jp  z,ncrboot           ;  -- the NCR 5380 SCSI disk
```

The three ROMs differ in two rows:

| `sw & 0f8h` | `mon375.s`, `mon447.s` | `mon500.s` |
|---|---|---|
| `0x00` | `boothd` — HDCA Winchester | `ideboot` — IDE |
| `0x08` | `nuboot` — DMA hard disk | `nuboot` — DMA hard disk |
| `0x10` | `djdma` — DJ-DMA floppy | `djdma` — DJ-DMA floppy |
| `0x18` | — (no such entry) | `ncrboot` — NCR 5380 SCSI |

`mon500` is the later ROM. Its all-switches-on position was re-pointed at an
IDE drive, and `boothd` and its entire HDCA register sequence are **gone from
the source** — `grep boothd mon500.s` finds nothing. The HDCA Winchester is
reachable only under the older two ROMs. `0x18` is the one row `mon500` has
that the others do not: it was added to spend the budget the on-board
diagnostics left when they came out.

A fourth decision is folded into the same byte: **bit 2 (`0x04`) is tested
separately at `check`, and if it is set the monitor is skipped** and the
machine goes straight to the booted program. `bit 2,a / jr z,montor`.

The selection also becomes the entry address. `tstsw` computes `de = (sw &
0f8h) << 8` and stores it as the new task's saved PC, then the task switch
writes a PC-swap sequence into `gobuff` and executes it, "because when the
task register is written into, the lower half of the prom goes away". Each
boot routine *overwrites* `de` before falling into `check`, so this default is
the monitor's, not the loader's.

The simulator presents this as `-c`: the low byte of the config value is
written straight to the switch register (`switchreg = config_sw & 0xff`), and
the higher bits are the simulator's own (`0x100` = put uart 0 in an xterm).
`-B <device>` names a boot device by name instead — `bootdevs[]` in
`hwsim.c` maps `hdca`→`0x00`, `ide`→`0x00`, `hdcdma`→`0x08`,
`djdma`→`0x10`, and ORs in `0x04` to skip the monitor. `m16boot` uses
`-c 0x10c`: `0x08` (HDC-DMA) + `0x04` (skip the monitor) + `0x100` (uart).

---

## 2. The five ways in

All five land a first level somewhere, and the first level's job is always
the same: get a second level into memory and jump to it. What differs is how
much room the ROM gives it — and that is decided entirely by which controller
did the reading.

| path | ROM entry | mechanism | first level gets |
|---|---|---|---|
| **HDCA** Winchester | `boothd` | the ROM drives the controller's ports itself, reading a header off the disk | **256 bytes** at the address the disk names |
| **HDC-DMA** hard disk | `nuboot` | the ROM builds a channel program and polls it | **512 bytes** at `0x0100` |
| **DJ-DMA** floppy | `djdma` | the *controller's own firmware* does the read; the ROM only polls | **128 bytes** at `0x0080` |
| **IDE** | `ideboot` (mon500) | the ROM drives an 8255 and the drive's task file | **512 bytes** at `0x0100` |
| **NCR 5380** SCSI | `ncrboot` (mon500) | the ROM drives the 5380's registers, one REQ/ACK byte at a time | **512 bytes** at `0x0100` |

The asymmetry is the whole story of why the floppy's first level has to be
tiny and the others do not.

### 2.1 HDCA — `boothd`

The oldest and the only *self-describing* one. The ROM never consults a
channel program; it drives the controller's registers directly
(`ioaddr equ 120Q` = `0x50`; `contrl`/`status` = `0x50`, `commd`/`secstat` =
`0x51`, `functn` = `0x52`, `data` = `0x53`).

The sequence, in `mon447.s:778`:

1. Select drive A and enable the drive (`functn` ← `drivea` 374Q, `contrl` ←
   `drenbl`).
2. Enable the controller (`dskrun`), then step the head out until the
   track-0 status bit appears (`stepo` 370Q), waiting on `complt` after each
   step.
3. Wait for the index pulse twice — the second wait is the head-settle delay.
4. Reset the buffer pointer to the header area, then hand-write a four-byte
   header: head 0, track 0, sector 1, and the "system key" (`system` 200Q).
5. Issue `dread` (1) into `commd` and wait for `opdone` (2).
6. Read **two bytes** from `data`: *the bootstrap address the disk itself
   carries.*
7. Read **256 bytes** to that address, one byte at a time, until the low byte
   of the address wraps.

Step 7 is the interesting one, because of §0. If the named address is below
`0x1000` it is not main memory at all, so it is rebased into the supervisor
window: `d = (d & 0fh) | 010h`, i.e. `0x1000 | (addr & 0xfff)`. An address of
`0x0100` becomes `0x1100` — which is the same byte as `0x0100` in task 1, and
which is exactly the `bootad equ 1100h` equate. An address already at or above
`0x1000` is used as it stands and the window is closed again
(`ld (mapram + 2),a`).

Then `de`/`hl` hold the address, `a` is 1, and it falls into `check`.

The HD-DMA controller's parameters are in the ROM as equates, and they are
the geometry everything else agrees with: `cyl 153`, `heads 4`, `secsiz 3`
("512 byte sectors for Micronix"), `stpdly 01eh`, `hdsetl 0c8h`. This is the
`m5:153:4:17` row of `stand/boot/GNUmakefile`'s `DRIVES`.

### 2.2 HDC-DMA hard disk — `nuboot`

The ROM is a bus master here in the other direction: it writes a channel
program into main memory and polls it, the same protocol the floppy uses but
with the ROM supplying the program.

```
nuboot: ld  bc,endboot - bootbl
        ld  hl,bootbl
        ld  de,chan             ; 1080h, physical 0x80
        ldir                    ; move the command block down
        ld  hl,iopb             ; 1050h
        ld  (hl),80h            ; channel address := 80h
        ...
        out (dmarst),a          ; reset the controller (port 0x54)
        call cloop              ; wait for it
        ...                     ; home: seek with ffff step pulses
        ...                     ; then ldir rdtbl..endrd (the read command)
        call cloop
        ld  de,0100h            ; "point to beginning of DMA boot prog."
        jp  check
```

The DMA address is `bootad` = `0x1100` in the window, i.e. physical `0x0100`,
and the length is a **full 512-byte sector**. That is why `mwboot1.s` — the
hard-disk first level, 181 bytes — has 512 bytes of room, and why it can
afford to relocate itself to `0xc000` before it starts loading the second
level on top of `0x0100`.

The command block is `bootbl..endboot` and the read block `rdtbl..endrd`;
both are in the ROM image, at `mon447.s:981` and `mon447.s:1002`.

### 2.3 DJ-DMA floppy — `djdma`

**The ROM does no I/O at all.** The DJ-DMA is an intelligent controller with
its own Z80 running the DJDMA25 firmware, and *it* performs the bootstrap
before the host's ROM gets a say.

Its own firmware bootstrap is real code, and we have its source:
`extra/hardware/djdma/Morrow/decision/djdma_firmware/DJ49.MAC` ("DJDMA
Rev 2.049 Rel_2.3 9_Aug_83", Copyright 1983 Morrow Designs). At line 220 it
is the routine `IPLPAT`/`HNDSHK`, assembled at `.PHASE 038H`, i.e. it runs at
host address `0x0038`:

```asm
HNDSHK: LD   HL,HFLG        ; 21 4A 00   HFLG is at 0x004a
        LD   (HL),0         ; 36 00
HLP1:   LD   A,(HL)         ; 7E
        OR   A              ; B7
        JP   Z,HLP1         ; CA 3D 00
        CP   40H            ; FE 40      "Normal_Completion"
        JP   NZ,HLP1        ; C2 3D 00
        JP   80H            ; C3 80 00   --> 0x0080
HFLG:   DB   0FFH           ; FF
```

Nineteen bytes. The reset path loads it with `LD DE,3813H` — `D := 38h` (start
address), `E := 13h` (length, 19 decimal) — after first reading the low 38
(`0x26`) bytes of host memory into its own RAM at `STACK 1030H` and zeroing
them.

Then it reads, and when the read succeeds it posts the answer by writing
*into host memory through its own I/O space*:

```asm
        LD   A,(IY+0CH)
        LD   (1324H),A      ; sector number := lowest sector number
        CALL RDSECT         ; read 1st sector on track 0 side 0
        CALL NZ,HSYNC
        LD   A,40H          ; bootstrap completion code
        LD   BC,HFLG        ; BC := pointer to main memory
        OUT  (C),A          ; main memory := operation complete
```

**The contract, then: 128 bytes of the first sector of track 0 side 0 are
loaded to `0x0080`, and the status byte at `0x004a` is set to `0x40`.** The
count is fixed at 128 — it is not "one sector" and it is 128 even when the
sector is 512. The controller then restores the 38 saved bytes.

The ROM's side is a variant of that same handshake, and the difference is
instructive:

```asm
djdma:  ld  h,10h                    ; wait byte for 1 minute
djlop0: ld  bc,0000h
djloop: ld  a,(djstat)               ; 104ah = physical 0x4a
        cp  040h
        ld  de,(djstat - 2)          ; 1048h = the JP's own operand
        jr  z,check
        cp  0ffh
        jr  z,nstat
        ...
nstat:  xor a
        ld  (djstat),a               ; null status byte ... signal DJ-DMA
```

`djstat-2` = `0x1048` is physical `0x0048`, which is where the `JP 80H`'s
address operand sits in the handshake routine above. **The ROM reads the entry
address out of the controller's handshake code rather than hardcoding
`0x0080`** — hence the comment "adjusted channel address of status byte".
`nstat` is the other half: when the controller has nothing to say it reads
`0xff`, and the ROM writes a zero back to unblock it.

`DJBOOT.md` §2 quotes the DJ/DMA Technical Manual's own wording for the same
contract, and §5-§6 work the resulting cascade out of the disk images.

### 2.4 IDE — `ideboot` (mon500 only)

Added in the last ROM. It reaches an 8255 and the drive's task file, and
because it has to cross the `0x1000` boundary it is the one that spells the
window out in a comment. It resets the drive by asserting `/RST` through port
C bit 7, with a delay loop that "has to contain a real bus cycle, so it reads
port A: the drive is not driving it, but the read is what keeps the loop from
being dead code", then loads the first sector to `0x1100` and jumps to
`0x0100`.

The tree's `ideboot1.s` is 243 bytes — large, for a first level — because
BIOS parameter blocks and LBA arithmetic cost more than a controller that
already knows how to find sector 1.

### 2.5 NCR 5380 SCSI — `ncrboot` (mon500 only)

Added to spend the budget the diagnostics left when they came out (the commit
that removed them says so). The card is eight registers at `0x40` — `CSD`
data, `ICR` initiator command, `MR` mode, `CSBS` status — driven by hand with
no DMA and no arbitration: the ROM resets the bus, selects target 0, sends a
ten-byte `READ(10)` for LBA 0, and reads 512 bytes one REQ/ACK round trip at
a time. Two subroutines carry the bytes — `ncrout` (command) and `ncrin`
(data) — and the wire they drive is exactly `sys/ncr.c`'s protocol.

The tree's `ncrboot1.s` is 254 bytes, the largest of the four first levels:
the byte handshake and the per-sector selection cost more than the IDE task
file. To fit the 256-byte budget ahead of the label it drops its sign-on —
the second level prints the banner — and its selection-timeout message. The
selection's "wait for the bus to be free" is one read, not a poll: a command
ends one status read after the read that saw REQ low, so one `in a,(scsistat)`
is all it takes.

---

## 3. The second level, and the handoff

Whichever way in, the first level's job is to fetch the second level and jump
to it. The second level is always at `0x0100` in *main memory*, because by
the time it runs the window is gone and `0x0100` means `0x0100`.

The second level is a Micronix filesystem-aware loader: it reads the root
inode, walks the directory, finds the kernel file, and loads its text and
data. In the tree's implementation (`stand/boot/boot.c`) that is `main()` =
`reset(); select(); load();`, with `#define BUF0 0x1000` and
`int (*loadbase)() = 0x1000;` — the kernel's load address, and the reason
`stand/boot/README` says "it's a good idea to keep these executables under
4k."

Then the handoff:

```asm
; sexit.s
_enterk: ld  ix,(_loadbase)
         ld  hl,(_inumber)
         jp  (ix)
```

**HL carries the inode number the loader booted from, and IX the entry
point.** The kernel stashes it in `_kino` — the loader hands the kernel the
inode it was booted from, so the kernel can reopen the file its overlay pages
ride in. `inumber` starts at 1 (the root) because HL zero means "the loader
chose nothing."

That is the whole design: the boot file is both the kernel and the container
of its own overlay pages, and the inode is the only thing that has to survive
the jump.

---

## 4. The tree's modern chain

`src/micronix/stand/boot/` reimplements the second level for all four
devices and shares one C file between them.

| target | size | role |
|---|---|---|
| `mwboot1.s` | 181 B | HDC-DMA first level |
| `djboot1.s` | 334 B | DJ-DMA levels 0 and 1 — one file, split at byte 128 |
| `ideboot1.s` | 243 B | IDE first level |
| `ncrboot1.s` | 254 B | NCR 5380 first level |
| `mwboot.com` | 3149 B | HDC-DMA second level |
| `djboot.com` | 2509 B | DJ-DMA second level |
| `ideboot.com` | 3448 B | IDE second level |
| `ncrboot.com` | 3856 B | NCR 5380 second level |
| `djload` | 5120 B | 10 × 512 B: level 0 in block 0, level 1 in block 1, `djboot.com` from block 2 |
| `bootimg-m*` | 3661 B | the HDC-DMA images, one per `DRIVES` row |
| `idebootimg-m*` | 3960 B | the IDE images |
| `ncrbootimg-m*` | 4368 B | the NCR 5380 images |

`CSRCS = boot.c mwio.c djio.c ideio.c ncrio.c`, `ASSRCS = sexit.s`. The second
level links `-Ttext=0x100 -L$(CCCLIB) sexit.o boot.o mwio.o` plus libc/libu/libc;
`boot.c` is shared and only the `*io.c` differs. The first levels are
assembled, linked, and then stripped of their 16-byte object header by
`dd ... bs=16 skip=1`.

Four of the five ways in need only one first level, because there the ROM is
level 0: it reads a whole 512-byte sector and jumps. The DJ-DMA cannot — it
delivers 128 bytes to `0x80` — so `djboot1.s` is two levels in one file. Its
first 128 bytes are a level 0 that does nothing but read block 1 and jump into
it; the level 1 behind them is `mwboot1.s` with a channel program for a read.
The file therefore links at `BASE0 equ 0080h`, and level 1's labels — which
the linker counts from there — come out at `0x100` only if level 0 and its pad
are exactly 128 bytes. That is the one silent-failure mode in the file, so
the `djload` recipe asserts it: byte 128 of `djboot1.bin` must be level 1's
first opcode, `0x21` (`ld hl,BASE`).

`DRIVES = m5:153:4:17 m10:306:4:17 m16:306:6:17 m32:640:6:17 m40:733:5:17`
— cylinders, heads, sectors. The geometry is baked into the image because a
second level that has to find a kernel on a bare disk must know the shape of
the disk to do the arithmetic.

Three layout constraints worth stating:

- **The first block also holds a `struct dlabel`** — the geometry — at byte
  256 from the start of the disk, the second half of block 0. `ideboot1`'s
  own comment puts it: "has to be shorter than the other two - the label sits
  at byte 256 of the sector and `mkbootimg` refuses an image whose first level
  runs into it." (`ncrboot1` lives under the same ceiling, and has the
  tightest fit of all: 254 of the 256 bytes.)
- **The floppy's image is laid out around that same byte**, and that is why
  its level 1 cannot share block 0 with level 0: level 1 is 206 bytes and
  would run from 128 to 333, straight through 256. So block 0 is level 0 with
  the label at 256, block 1 is level 1, and blocks 2 through 9 are the second
  level — one side of the ten-sector cylinder-0 track. The label itself is
  left zeroed, which reads as "no label on the drive, use your own geometry"
  (`sys/dlabel.h`); nothing on the floppy path reads one yet.
- `djio.c` carries the fixed geometry the floppy's filesystem is written in:
  `CYLINDERS 40`, `HEADS 2`, `SPT 10`, `TOFF 2`, `LIMIT
  ((CYLINDERS-TOFF)*SPC-1)`, and the comment "the filesystem's block 1, the
  superblock, is block 40 on the disk, and the block numbers boot.c hands
  readblock are 39 short of the disk's." Note that `cmd/djformat` and
  `DJBOOT.md` §5 each give a different drive — 77 tracks, and 77 cylinders
  with one head — so which geometry a floppy label would carry is open.

`mkfs -i` is what puts the two levels on the disk: "it puts the two of them
in a file that owns cylinder 0, block 0 being the first level and block 1
onward the second." `djload` is the floppy's counterpart, built by the
Makefile rather than by mkfs, since the first two levels are one file.

---

## 5. The CP/M side, and the hard disk that holds no boot block

Historically there were two ways in, and **neither of them puts anything on
the hard disk.** `disks/loaders/coldboot/README`:

> two ways in, and neither of them puts anything on the hard disk

1. **Under CP/M**, run `m5boot.com` (or `m10boot.com`, `m16boot.com`,
   `djboot.com`). These are ordinary `.com` files, loaded by CP/M at
   `0x0100`, which then do the bootstrap themselves. `disks/loaders/coldboot/`
   holds them: `M5BOOT.COM` 3712, `M10BOOT.COM` 3584, `M16BOOT.COM` 3712,
   `DJBOOT.COM` 3840, `HDBOOT.COM` 4480, with their `*LOAD` counterparts.
2. **From a floppy**, sysgen `m5load` (or `djload`, …) onto the diskette's
   system tracks and reset the machine. The controller reads the floppy, the
   floppy's loader reads the hard disk.

The `*LOAD` images all share a layout: `0000`–`07ff` zero, the **first level
as 128 bytes at `0800`** (so it runs at `0x0080`), and the **second level at
`0a00`** starting with its own crt0, `ld sp,0100 / call ... / jp 0`.

> The first level code is common to all of them — M5LOAD's first 92 bytes at
> 0800 are the floppy's exactly, and 92 is 5c, which is where the channel
> program starts. Same code, different channel program.

Dumping `0x800`–`0x8ff` out of `DJLOAD`, `M5LOAD`, `M10LOAD`, `M16LOAD` and
`HDLOAD` confirms it: **all five are byte-identical there.** What differs is
the channel program the first level plants.

That first level is a *later* design than the one in the 1.3/1.41 images. It
is READTRK-based rather than SREAD-based, and it uses the 24-bit DMA wrap:

```
SETDMA 0xFFFF00
READTRK cyl 0               ; sectors 1..9 land at 0x0100..
SETDMA 0x001300
READTRK cyl 1
HALT
```

Reading to DMA address `0xFFFF00` with 512-byte sectors puts sector 0 off the
top of the memory space and sector 1 at `0xFFFF00 + 0x200 = 0x1000100`, which
wraps in 24 bits to `0x000100`. So sectors 1–9 land contiguously at
`0x0100`–`0x12FF` and the next track continues at `0x1300`. The trick is
documented in the Morrow CP/M cold boot loader
(`src/hwsim/resources/cpm22/E3/ABOOT&.ASM`), which carries the Micronix
variant behind `micron equ 1`:

```
cboot  equ 0100h   ; cold boot address for the loader
loaddr equ 0100h
db 0ffh            ; wrap around from ffff00 to 000100
```

Its channel program sits at `0x00BE` *inside* the 128-byte boot sector, with
the flag byte at `0x00D7` — which is why the five `*LOAD` files look
identical for 256 bytes but are not: the differing bytes are past `0xBE`.

The earliest design, in `disks/boot.IMD` and `disks/dist/UX141_SA.IMD`
(Micronix 1.41 standalone, 8-inch, "micronix 1.3 #1010-8 / stand alone /
1982 gary fitts"), uses SREAD instead. `disks/boot.IMD`'s first level is
byte-for-byte the same block as `UX141_SA.IMD`'s, and decodes as: set up a
stack at `0x3080`, plant a BRANCH (`0x26`) into the channel reset address,
run two small programs, then index a table of pairs at `0x00F5` with the
drive's sector-length code and jump to `0x3080`. `DJBOOT.md` §5 walks the
whole cascade (stages 0–4) out of that image.

---

## 6. The sector-size hazard

Worth stating on its own because it is silent. CP/M wants the drive type's
**native** sector size — 1024 bytes for the ST-506 class this machine uses —
and Micronix requires **512**. `src/hwsim/DISKS` records the hazard and how
each controller deals with it:

- **HDCA** is fixed at 512 (`SECLEN`), and works around a block that straddles
  a sector boundary with an explicit byte shuffle:
  `bcopy(&iobuf[0], &buffer[2], 510); bcopy(&iobuf[510], &buffer[0], 2);`
- **HDC-DMA** has a programmable sector size, which is why the ROM's equates
  can say `secsiz equ 3 ;512 byte sectors for Micronix`.
- A mismatch between what the drive was formatted with and what the reader
  assumes is **silent corruption, not an error.**

This is the origin of the standing constraint that DJ-DMA filesystems are and
will always be 512-byte sectors — the boot ROM requires it — and of the work
to move every non-512-byte transfer out of the kernel driver and into the
program that actually knows the medium.

---

## 7. Open questions

- ~~**`djboot1.s` does not fit the DJ-DMA contract.**~~ **Fixed.** The
  controller loads 128 bytes to `0x0080`; `djboot1.s` was 206 bytes linked at
  `0x0100` and written against `nuboot`'s behaviour rather than `djdma`'s, so
  it could never have run. It is now two levels in one file: a 128-byte level
  0 at `0x0080` that chain-loads, and the old level 1 behind it at `0x0100`,
  which is where the level 0's `SETDMA` puts it. The `djload` recipe asserts
  the 128-byte boundary. `DJBOOT.md` §6 has the contract.
- **What geometry a floppy label would carry.** The label belongs at byte 256
  from the start of the disk and the new layout leaves it free, but nothing
  writes one and the three sources disagree about the drive: `djio.c` says 40
  cylinders / 2 heads / 10 spt, `cmd/djformat` says 77 tracks, `DJBOOT.md` §5
  says 77 cylinders / 1 head with track 0 FM 26×128 and 15×512 after. A label
  exists to settle exactly this, so it waits on the answer.
- **No image can boot the DJ path end to end.** `djdma_init` loads through
  `imd_load`, so it wants an IMD; `mnix bootflop` writes a raw 800-sector run,
  and there is no raw-to-IMD writer in `src/tools`. `mnix initialize`'s
  `mediums[]` has no 5¼" entry either. So `djload` is verified only as far as
  its own block map — level 0 to level 1 — and level 1's read of level 2 is
  untested.
- **`disks/loaders/README` notes a remaining puzzle** about the 1.3 cascade:
  after the two reads, 512 + 128 = 640 bytes land at `0x3080`, and "the bytes
  at 3080 do not look like an entry point."

---

## 8. Where to read further

| document | what it holds |
|---|---|
| `DJBOOT.md` | the DJ-DMA floppy path in full: the manual's wording, the UX141_SA cascade, the `ABOOT&.ASM` variants |
| `INSTALLATION.md` | how to build and install the tree, and boot each card |
| `disks/loaders/README` | the two-stage floppy boot, the extraction commands, the 1.41 image's disassembly |
| `disks/loaders/coldboot/README` | the cold boot set, the two ways in, the `*LOAD` layout |
| `src/hwsim/DISKS` | the sector-size hazard, per controller |
| `src/micronix/stand/boot/README` | the tree's chain, and the 4K discipline |
| `src/micronix/sys/OVERLAY-DRIVERS.md` | what happens after the kernel is entered |

Sources for each path: `src/micronix/stand/roms/mon{375,447,500}.s` and their
equate blocks; `src/hwsim/d1/djdma.c` (`bootstrap[]`, `djdma_init()`) and
`hdca.c`/`hddma.c`/`ide.c`/`ncr5380.c`; `src/hwsim/d1/mpz80.c` (`SWT`, the `SW_*`
defines, `switchreg = config_sw & 0xff`); `src/hwsim/hwsim.c`
(`bootdevs[]`); `extra/hardware/djdma/Morrow/decision/djdma_firmware/DJ49.MAC`
(the controller's own firmware); `src/hwsim/resources/cpm22/E3/ABOOT&.ASM`.
