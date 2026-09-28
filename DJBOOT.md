# The DJ/DMA boot process — CP/M and Micronix

A reference for how a Morrow Designs Disk Jockey DMA (DJ/DMA) floppy
controller boots a machine, in both its CP/M and Micronix guises, and how
the simulator (`hwsim`) models it.

The single fact everything downstream hangs on: **the DJ/DMA bootstrap loads
a fixed 128 bytes (0x80) of the first sector of track 0 to `0x0080` and
branches the CPU to `0x0080`.** The count is fixed — it is *not* "one
sector", and it is 128 bytes even when the sector is 512. Every boot loader
for this controller has to live in that window or stage its way out of it.

---

## 1. The controller

The DJ/DMA is an intelligent floppy controller with its own Z80, running
the DJDMA25 firmware. The host does not bit-bang the drive; it writes a
**channel program** — a sequence of commands — into S-100 bus memory and
pulses the attention port. The controller's Z80 fetches and runs the
program, writing status bytes back into the channel as it goes.

- default channel command address: `0x0050` (`DEF_CCA` in `djdma.c`)
- attention/kick port: `0x00ef` (`DJDMA_PORT` / `djkick`)
- command codes (`djdma.c` `djcmd[]`, `djboot1.s`, `ABOOT&.ASM`):

| code | name      | bytes | what it does |
|------|-----------|-------|--------------|
| 0x23 | SETDMA    | 4     | set 24-bit DMA address (lo, hi, ext) |
| 0x20 | SREAD     | 5     | read one sector (cyl, sec, drive, status); sec bit 7 = head |
| 0x21 | WRITESEC  | 5     | write one sector |
| 0x22 | SENSE     | 6     | sense drive (drive, dcb, slc, dsb, status); slc = sector length code |
| 0x29 | READTRK   | 8     | read a whole track (cyl, head, drive, sectab, status) |
| 0x2a | WRITETRK  | 8     | write a whole track |
| 0x25 | HALT      | 2     | end the channel program |
| 0x26 | BRANCH    | —     | branch in channel (address in following 3 bytes) |
| 0x27 | SETCHANNEL| 4     | set the default channel address |
| 0x2e | SETDRIVE  | 3     | set logical drive mapping |

- status: `0x40` = good (`S_NORMAL` / `OKSTAT`); `0x80` and up are errors
  (`S_ILLREQ`, `S_ILLDRV`, `S_UNREADY`, …).
- `READTRK` walks a **sector table** (one byte per sector): `0xff` = skip,
  anything else = load, and the controller writes the per-sector status back
  into that byte.
- SETDMA addressing: if the extended byte is 0, it is a flat 16-bit
  address; otherwise `seg<<12 | (offset & 0xfff)`. The boot loaders exploit
  the extended byte to wrap the address (see §5).

## 2. The bootstrap — the 128-byte window

From the **DJ/DMA Technical Manual, Revision 1, April 1982**, §6.2
"Bootstrap Load" (and `DJ_DMA_Preliminary_Technical_Manual_Jan82.pdf`):

> At reset or power-on … the controller halts the main CPU by taking
> control of the bus and reads the first 38 (hex) locations in main memory
> into its own local memory. Next it loads 0s into these first 38 (hex)
> bytes and places a short, 19 byte (decimal) handshake routine between
> 000038 and 00004A (hex). … Next, **80 (hex) bytes are loaded between
> 000080 and 0000FF (hex) from the first sector on Track 0 of the disk.**
> Finally, the controller writes a control byte to the handshake routine
> which causes the main CPU to branch to location **000080** (hex).

The 19-byte handshake (manual Table 7-1, reproduced verbatim in
`djdma.c` `bootstrap[]`):

```
0038: 21 4A 00   ld hl,004a
003B: 36 00      ld (hl),0
003D: 7E         loop: ld a,(hl)
003E: B7         or a
003F: CA 3D 00   jp z,loop
0042: FE 40      cp 40h
0044: C2 3D 00   jp nz,loop
0047: C3 80 00   jp 0080h
004A: FF
```

The CPU spins on the status byte at `0x004a` until the controller writes
`0x40` into it, then jumps to `0x0080`, where the 128-byte cold boot loader
waits.

The simulator models exactly this in `src/hwsim/d1/djdma.c`, `djdma_init()`:
save/zero `0x0000–0x0037`, plant `bootstrap[]` at `0x0038`, then

```c
imd_read(imdp[physdrive(0)], 0, 0, imd_firstsec(...), secbuf);
copyout(secbuf, 0x80, 0x80);   /* 128 bytes to 0x0080, always */
physwrite(0x4a, 0x40);
```

## 3. The monitor's side — mon447 / mon375

The Decision 1 monitor (`extra/sim/decision1/rom/mon447.s`, `mon375.s`)
has two disk-boot entries, and they are not symmetric.

- **`nuboot`** (HD-DMA hard disk): the monitor builds a command block and
  reads a **full 512-byte sector to `0x0100`**, then jumps to `0x0100`
  (`rdtbl` has DMA `00 01`; `rdata` does `ld de,0100h / jp check`). This is
  why the hard-disk first level (`mwboot1.s`, 181 bytes) gets 512 bytes of
  room at `0x0100`.

- **`djdma`** (floppy): the monitor does **not** read anything. It polls
  `djstat` = `0x104a` for `0x40`, reads the boot address from `djstat-2` =
  `0x1048`, and jumps to it (`mon447.s:623-650`). The address the
  controller's bootstrap leaves there is `0x0080`. The floppy first level
  therefore gets only the controller's fixed 128 bytes at `0x0080` — there
  is no "read a full sector to 0x0100" path for the floppy.

That asymmetry is the whole story of why the floppy first level has to be
tiny and the hard-disk one does not.

### The address space at the branch

What is actually in memory the moment the first level starts running is
fixed, and the loaders are written against it exactly.

For the **floppy** (branch to `0x0080`):

```
0x0000–0x0037   zero. The controller saved the original contents into its
                own local RAM and zeroed these, and restores the originals
                once the handshake has run. A CPU whose reset vector is
                0x0000 reaches the handshake by sliding through these NOPs;
                on the Decision 1 the monitor's `djdma` routine is the
                intermediary instead.
0x0038–0x004a   the 19-byte handshake the controller planted — it polls
                0x004a, then `jp 0080h`. The byte at 0x004a is the status
                byte the controller set to 0x40 when the sector arrived,
                and 0x0048–0x0049 hold 0x0080 (the `jp` operand), which is
                where the monitor's `djdma` reads the boot address from.
0x0080–0x00ff   the cold boot loader — the only 128 bytes the bootstrap
                has actually loaded.
0x0100 …        not loaded. Whatever RAM holds there is uninitialised; the
                loader must fetch everything above this itself.
```

The stack is not set up, there is no argc/argv and no environment — the
loader is entered with bare registers. That is why a floppy first level is
128 bytes at `0x0080` and a blank slate from `0x0100` up, and why it must
build its own stack before it can `call` anything (the CP/M loader avoids
`call`/`ret` entirely and uses only `jp`, so it never needs a stack; the
Micronix stage 1 uses `call` and so does `ld sp,3080h` first).

For the **hard disk** (`nuboot` branch to `0x0100`) the picture is the
simpler one: the monitor read a full 512-byte sector to `0x0100–0x02ff`,
`0x0000–0x00ff` is untouched monitor RAM, and everything from `0x0300` up
is the same blank slate.

The monitor's constants are one step removed from the physical addresses
above: its task-0 view of the controller's S-100 memory is shifted up by
`0x1000`, which is why `djstat` is `0x104a` (physical `0x004a`) and the
channel it programs is `0x1080` (physical `0x0080`).

## 4. The CP/M cold boot loader — ABOOT

The Morrow CP/M 2.2 cold boot loader source is
`src/hwsim/resources/cpm22/E3/ABOOT&.ASM` ("Morrow Designs CP/M vers 2.2
Cold Boot Loader", CBIOS revision E.2/E.3). For the DJ/DMA it assembles
with `boot equ 80h` and is written to fill the 128-byte window exactly:

- plant a BRANCH (`0x26`) at the channel `0x0050` pointing at the program;
- the program is `SETDMA` + `READTRK` track 0 + `READTRK` track 1 + `HALT`,
  loading the whole system (CCP + BDOS + CBIOS) in two whole-track reads;
- a sector table (`0xff` = skip) marks which sectors are the boot loader
  and which are the OS; on success it jumps to `cboot` (the BIOS cold-start).

The same file carries the Micronix variant behind `micron equ 0/1`. With
`micron equ 1`:

```
cboot  equ 0100h   ; cold boot address for the loader
loaddr equ 0100h
...
db 0ffh            ; wrap around from ffff00 to 000100
```

— i.e. the Micronix second level lands at `0x0100`, and the loader reaches
`0x0100` through a 24-bit DMA address whose high/extended bytes are `ff`, so
the READTRACK wraps around the top of memory back into the first 64K. The
comment "Provisions have been made for a Micronix boot loader. This loader
always gets loaded to 0100h." is the origin of that address.

The layout table in the file's header (8-inch, DJ2D/B; the DJ/DMA loads the
same map in one READTRACK) is: track 0 sector 1 = boot loader, sectors 2–4
spare, sectors 5–26 = CCP + BDOS, track 1 = CBIOS.

### Confirmed on `926.IMD`

`disks/dist/926.IMD` (`48k cp/m version 2.2 cbios revision e.3 #926`,
`ssdd`) is the disassembled confirmation. Geometry: **track 0 = FM 26 × 128,
track 1 = MFM 8 × 1024.** Its 128-byte cold boot loader at `0x0080` is one
stage and does the whole job:

```
0080: ld a,26h ; ld (50h),a      channel[0] = BRANCH
0085: ld hl,0beh ; ld (51h),hl   channel[1..2] = 0x00be (the program)
008b: xor a ; ld (53h),a         channel[3] = 0
008f: out (0efh),a               one kick
0091: ld a,(0d7h) ; or a ; jp z,0091h   poll the HALT status
0098: ld hl,0ddh ; ld bc,40ffh ; ld de,22h   sector table, 34 entries
00a1: …check each entry, count errors, retry…
00b4: jp z,0ac00h                → the CBIOS cold start

; channel program (0x00be):
00be: SETDMA 0x9400 ; READTRK cyl0 (sectab 0xdd)   ; track 0 → 0x9400
00ca: SETDMA 0xa100 ; READTRK cyl1 (sectab 0xf7)   ; track 1 → 0xa100
00d6: HALT
```

Track 0 sector 5 (loaded to `0x9400`) opens with the CCP jump table and the
`COPYRIGHT` banner, confirming the load lands the OS directly at its final
address.

**Structural contrast with Micronix.** The CP/M loader is a closed image —
CCP, BDOS and CBIOS fit in two tracks, so the 128-byte cold boot can dump
them straight into high memory (`0x9400`/`0xa100`, flat DMA) and jump into
the OS (`0xac00`). Micronix is open: the kernel is a file in a filesystem,
so the cold boot must bring in a *loader that understands the filesystem*
(3224 B, too big for the window), and that forces the two-stage staging
cascade. Concretely:

| | CP/M (926.IMD) | Micronix (UX141_SA) |
|---|---|---|
| stages | 1 (128 B is the whole cold boot) | 2 (128 B stages a 512 B continuation at `0x3080`) |
| destination | high memory, flat DMA (`0x9400`, `0xa100`) | low memory, 24-bit wrap (`0xffff00 → 0x0100`) |
| what it loads | the whole OS in two tracks | a second-level loader, which then reads the kernel |
| final jump | into the OS (`0xac00`, CBIOS cold start) | into a loader (`0x0100`), which loads the kernel to `0x1000` |
| SENSE | none (fixed geometry) | SENSEs and patches the sector table by sector size |
| track 1 sectors | 1024 B | 512 B |

## 5. The Micronix boot — the UX141_SA cascade

`disks/dist/UX141_SA.IMD` (Micronix 1.41 standalone, 8-inch) is the worked
example, disassembled from the image itself.

Disk geometry: 77 cylinders, 1 head. **Track 0 is FM, 26 × 128-byte
sectors; track 1 onward is MFM, 15 × 512-byte sectors.** Tracks 0–1 are
reserved for the boot; the filesystem starts at track 2.

| # | stage | length | disk | memory | function |
|---|-------|--------|------|--------|----------|
| 0 | DJ/DMA bootstrap | firmware | controller ROM | loads to `0x0080` | load 128 B of sector 1, branch to `0x0080` |
| 1 | cold boot loader | 128 B | track 0 sector 1 | `0x0080–0x00ff` | fetch stage 2 to `0x3080`, sense the drive, jump `0x3080` |
| 2 | cold boot continuation | 512 B | track 0 sec 3 + track 1 sec 1 | `0x3080–0x32ff` | READTRACK stage 3 into `0x0100`, jump `0x0100` |
| 3 | second-level boot loader | 3224 B | track 0 sec 5–26 + track 1 sec 1–2 | `0x0100–0x0d97` | read the kernel file off the filesystem, load `0x1000` |
| 4 | kernel | filesystem file | track 2+ | `0x1000` | the OS |

**Stage 1** (128 bytes, exactly `0x0080–0x00ff`; 91 bytes of code + a
37-byte channel program/table). It only stages the next step: plant BRANCH
at the channel, kick `SETDMA 0x3080; READ cyl1/sec1; SENSE; HALT`, then
`READ cyl0/sec3; HALT`, index the SENSE sector-length code into a 4-entry
table to patch stage 2's sector table, and `jp 3080h`.

**Stage 2** (512 bytes at `0x3080`). Entry is the 128-byte track-0 sector 3;
the other 384 bytes are the tail of track 1 sector 1. It reuses stage 1's
kick-and-poll subroutine (`call 0c0h`, still resident at `0x00c0`) and runs

```
SETDMA 0xffff00 ; READTRK cyl 0 (sectab 0x30c9)   ; wraps into 0x0000+
SETDMA 0x0c00   ; READTRK cyl 1 (sectab 0x30e3)   ; into 0x0c00+
HALT
```

then checks the sector table and `jp 0100h`. The `0xffff00` SETDMA is the
wrap that puts the second level at `0x0100`.

**Stage 3** (3224 bytes at `0x0100`, entered `ld sp,0100h`) is the real
loader: sector-by-sector `SETDMA`/`SREAD`, walks the Micronix filesystem on
tracks 2+, finds the kernel, loads it to `0x1000`, and jumps there. It is
what the tree's `djboot.com` (`boot.c` + `djio.c`) rewrites.

Stages 1 and 2 together are the "first level": the 128-byte sector is too
small to do the READTRACK-and-load itself, so it stages its own continuation
at `0x3080` and that continuation does the heavy load.

## 6. The tree's floppy first level

`src/micronix/stand/boot/djboot1.s` used to be a **206-byte** program linked
at `BASE equ 0100h` that relocated itself to `0xc000`, looped `SREAD` over
sectors 1–8 into `STAGE = 0x8000`, unpacked the Whitesmith object it found
there (`src/include/obj.h`: 16-byte header, magic `0x99`, text/data/bss +
textoff/dataoff), and jumped to its text.

It did not fit the DJ/DMA bootstrap contract in two independent ways — 206 >
128 bytes, and `0x0100` ≠ `0x0080` — and its own header comment, "The
monitor's DJ-DMA boot reads one sector to 0100", described `nuboot`'s
behaviour, not `djdma`'s. **It could never have run**, and never did: the
harness boots `hdcdma0:`, so nothing exercised the path.

It is now two levels in one file, which is the shape the contract forces.
Level 0 is the first 128 bytes and level 1 is everything after them, so the
file links at `BASE0 equ 0080h`:

- **level 0** — the 128 bytes the controller leaves at `0x0080`. No stack,
  no output, no retry: it writes the channel program's invariant half, does
  one `SREAD` of block 1 to `0x0100`, and `jp level1`. A failure loops on the
  status byte forever, because there is nothing to say it with and nothing
  else to do. It pads to exactly 128 bytes.
- **level 1** — the old program, unchanged but for `ld c,1` becoming
  `ld c,L2SEC` (2). It relocates `0x0100 → 0xc000`, `SREAD`s blocks 2–9 into
  `STAGE = 0x8000`, unpacks the object, and jumps to its text.
- **level 2** — `djboot.com`, `boot.c` + `djio.c` + `sexit.s` linked
  `-Ttext=0x100`. `boot.c` loads the kernel to `0x1000`
  (`loadbase`/`loadptr`); the kernel must not be touched by anything of the
  loader that reaches `0xff0`.

The 128-byte boundary is the one silent-failure mode: level 1's labels are
the linker's, counted from `0x0080`, so they come out at `0x0100` — where
level 0's `SETDMA` puts block 1 — only if level 0 is exactly the length the
firmware copies. Nothing about a wrong image would look wrong, so `djload`
asserts it: byte 128 of `djboot1.bin` must be `0x21` (`ld hl,BASE`). The
assembler shortens level 0's trailing `jp` to a `jr`, which is why the head
is 59 bytes of code and 69 of pad.

`djload` is ten blocks — one side of the cylinder-0 track at the tree's
512-byte-sector geometry:

| block | contents |
|---|---|
| 0 | level 0 (bytes 0–127); byte 256, the label's slot, left zero |
| 1 | level 1 (bytes 128+) |
| 2–9 | `djboot.com` |

Blocks 10–39 are the rest of the two reserved cylinders and are unused; the
filesystem starts at block 40. The label's slot is why level 1 has a block of
its own: 206 bytes from byte 128 would run 128–333, through 256, and `DL_OFFSET`
is a fixed 256 from the start of the disk.

**This is not UX141_SA's stage structure, and does not need to be.** That disk
is FM on track 0 (26 × 128-byte sectors) and MFM from track 1 (15 × 512), so
its stages 1 and 2 use `READTRK` with a sector table and a `SENSE` to patch
the sector size into it. The tree's format is 512-byte sectors throughout —
the standing constraint, the boot ROM requiring it — so level 0 has no table
to build and no size to sense: it reads one block by number, `SREAD`. The two
stages exist only because 128 bytes cannot hold the loader.

## 7. Files and floppies

Sources that say the real thing:

- `extra/docs/DJ_DMA_Technical_Manual_Revision_1_Apr82.pdf` — §6.2
  bootstrap, Table 7-1 handshake. (`..._Preliminary_...Jan82.pdf` is the
  earlier edition.)
- `extra/sim/decision1/rom/mon447.s`, `mon375.s` — the monitor's `djdma`
  and `nuboot` entries.
- `src/hwsim/d1/djdma.c` — the simulator's model (`djdma_init` bootstrap,
  `readsec`/`readtrk`/`setdma`/`sense`, `djcmd[]`).
- `src/hwsim/resources/cpm22/E3/ABOOT&.ASM` — the CP/M cold boot loader and
  its Micronix variant. (Siblings: `bdos.asm`, `ccp.asm`, `cbios-e4.asm`,
  `E3/CBIOSE3.ASM`, `E3/FORMATDJ.ASM`.)
- `src/micronix/stand/boot/djboot1.s`, `djio.c`, `boot.c`, `sexit.s`,
  `GNUmakefile` — the tree's floppy boot.
- `src/include/obj.h` — the Whitesmith object header the tree's first level
  unpacks.
- `extra/hardware/djdma/Morrow/decision/djdma_firmware/` — DJDMA25 firmware
  binaries (`DJDMA_V2.5_26C2.bin`, `DJDMA_11C.bin`, `DJDMA_12B.bin`,
  `DJDMA_3D.bin`).

Floppy images the cases were found in:

- **Micronix boot cascade**: `disks/dist/UX141_SA.IMD` (standalone 1.41;
  also `src/hwsim/disks/UX141_SA.IMD` and
  `extra/hardware/djdma/Morrow/micronix/1.4/UX141_SA.IMD`). Family:
  `disks/dist/UX14_D1.IMD`, `UX14_D2.IMD`.
- **CP/M cold boot**: `disks/dist/926.IMD` (the CP/M volume; disassembled in
  §4; also `src/hwsim/disks/926.IMD`), `1009-8_cold-boot.IMD`,
  `1010-8_stand-alone.IMD`, and the rest of the 9xx distribution series
  (`901`, `902`, `908`–`913`, `921`–`922`, `925`–`926`, `930`–`931`, `935`,
  `941-8`–`943`, `947-1`–`947-2`, `509`, `805`, `817`, `1070-8_pascal`).
- **Simulator boot floppies**: `disks/boot.IMD`, `src/hwsim/d1/boot.IMD`,
  `src/hwsim/d1/DRIVE_A.IMD`.

Tools used to read the images: `src/tools/imd` (`-s`/`-d`) and
`src/tools/dumpsec` (raw sector order), over `src/lib/imdlib.c` /
`src/include/imd.h`.
