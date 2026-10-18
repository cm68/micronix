# Legacy boot — the medium's own sector 0

A boot medium is one of two kinds, and four bytes in block 0 decide
which.  Ours carry the disk label (`sys/dlabel.h`): the magic MWDL at
offset 256 of block 0, which is what level 1 (`stand/roms/l1boot.s`)
looks for and what level 2 (`stand/boot/djboot1.s`) reads the geometry
out of.  The shipped CP/M diskettes and the Morrow Micronix 1.3 and 1.4
stand-alone and distribution diskettes carry nothing there: their block
0 is the medium's own boot sector, written to run at 0080h in task 1 —
which is where the floppy controller's *own* bootstrap has put it since
1983, and where its author could count on finding it.

The rule:

> the floppy code needs to read the disk, and if it finds a label, then
> it can know the load address is as expected.  if not, then it knows
> that it is dealing with a legacy boot sector like the one loaded by
> the djdma boot mode.  ...  if we don't see the label, we should move
> the boot code we loaded and jump to it.

This is the analysis behind that rule: what the stock machine did, byte
for byte, so the two contracts that matter — booting every distro
diskette, and generating the diskettes we ship — can be checked against
a written condition instead of rediscovered on the console.

## The four levels

    0   the monitor in its socket (mon375, mon447, mon500).  Finds the
        console, sizes memory, and dispatches on the switches.
    1   stand/roms/l1boot.s, 4k, org 0F000h.  Walks the controllers,
        finds a labelled disk, reads block 0, enters it.
    2   block 0 of the medium: djboot1.s, mwboot1.s, ideboot1.s,
        ncrboot1.s, hdcaboot1.s.  Knows the filesystem's shape, loads
        the loader.
    3   stand/boot/boot.c and one of the io drivers: sysfs, the menu.

Level 0 is where the two kinds of medium part company.  The monitor's
*own* way into a floppy — switch 4 off, `mon447.s`'s `djdma:` — is not
a reader.  It is the host half of a handshake with the controller's
firmware, and the firmware is what reads the disk, to a place and in a
shape the monitor never chooses.  That place is 0080h.

## The card's own bootstrap

The definitive source is the DJ-DMA card's firmware, Rev 2.049,
`src/hwsim/resources/DJDMA25/DJ49.MAC`.  It runs from power-on reset of
the S-100 bus (the card's POC), and everything below is from it.

It bootstraps only if a terminal or a shunt is seen on its own serial
port — `LD A,(4003H); AND 2; JP NZ,IMLOAD`.  With no terminal the card
brings up its internal monitor instead (`JP MAIN`) and the host is on
its own.  That is the first contract of the mode: *automatic* bootstrap
is conditional on the card's port, and the monitor's `djdma:` routine
is what makes it happen from the host's side.

    1.  IX := BASEIX (13F0h), IY := (BASEIX) — the drive table — and
        SP := STACK (1030h): all inside the card's own ram.  One
        second of delay for the bus to settle.

    2.  Main memory 0000h-0037h is copied into controller ram at
        1030h.  Then 38 zero bytes are written to main memory
        0000h-0037h, and the 19-byte handshake routine IPLPAT/HNDSHK
        is moved into main memory at 0038h-004Ah.

    3.  The card waits for the *host* to write a zero to the flag.

            IPLPAT:  .PHASE 038H
            HNDSHK:  LD   HL,HFLG     ;HL:= Pointer to Flag
                     LD   (HL),0      ;Flag:= 0
            HLP1:    LD   A,(HL)      ;Repeat
                     OR   A            ;(get Flag)
                     JP   Z,HLP1      ;Until (Flag ne zero)
                     CP   40H         ;If (Flag ne Normal_Completion)
                     JP   NZ,HLP1     ;     GOTO Loop
                     JP   80H         ;GOTO Cold Boot Loader
            HFLG:    DB   0FFH        ;THIS IS A FLAG TO THE LOADER
                     .DEPHASE

        So the flag is the byte at 004Ah and the boot address is the
        operand of the `JP 80H` at 0048h-0049h.  Reading them is the
        whole of what the host has to do: `mon447.s`'s `djdma:` loop
        polls 004Ah, and when it reads 0FFh — the stub's own initial
        value, or untouched ram — it writes a zero there (`nstat:
        xor a; ld (djstat),a`).  That zero *is* step 3's handshake;
        the monitor is standing in for the 19 bytes, which is why the
        real machine boots with the monitor in front of it and why the
        stub is only reached by a host that runs 0038h itself.

    4.  Main memory 0000h-0037h is restored from controller ram.  This
        is the point of the save and the zero: a legacy medium is a
        CP/M medium, its first instruction may be anywhere and it will
        expect page zero to be whatever the host had.  The save and
        restore is the card being polite to a host whose zero page is
        live.

    5.  BSTART: the dma address is set to 0080h (controller ram
        1320h-1324h: low byte 80h, then track, side, sector all zero,
        then the *start* sector number from the drive table), a drive
        is selected, its parameters installed, and

            CALL RDSECT   ;Read 1st Sector on Track 0 Side 0

        RDSECT reads **one sector**, at the drive's own sector length,
        from cylinder 0, side 0, lowest-numbered sector, to bus 0080h.
        On an eight inch drive that is FM, 128 bytes, 26 sectors,
        numbered 1 to 26 on a soft-sectored drive and 0 to 25 on a
        hard-sectored one — `HSYNC` is called first when the drive's
        flag says hard sectored, and the drive's lowest sector number
        is what goes in the sector register.  Every eight inch drive
        has an FM cylinder 0, whatever the rest of the medium is.

    6.  A failed read is not fatal and not reported: the code loops
        back to BSTART and tries the next drive.  The bootstrap
        *finds* the boot drive; no drive number is assumed anywhere in
        this path.

    7.  Success: 40h is written to the flag at 004Ah, the card's
        monitor goes inactive, and it goes back to its main loop.  The
        host, polling that byte, sees 40h and boots.

The shape of what the medium gets, then, is one sector's worth of
bytes at 0080h — not a 512-byte block, and not at 0100h.  A medium
whose sector 0 is 128 bytes has to read the rest of itself; a medium
whose sector 0 is 512 bytes (a five inch drive, where the drive's own
sector length is 512) has it all.  This is the difference that matters
against our own block 0, which is defined as 512 bytes with the label
at 256, and which level 1 assembles out of four 128-byte sectors when
it has to.

## The monitor's half

`mon447.s`, in its own words and registers.  The reset entry ends in
`reset0: call reset1 / call uartst / call setup / jp gobuff`, so the
switch dispatch happens *inside* `setup` and the monitor always leaves
reset by jumping to gobuff (01B0h) — the task switch stub, which by
then holds either the monitor's own switch or the boot's.

    getsw:  ...
    reset:  jp   z,reset0        ;S6 on: the monitor
            ld   a,(switch)
            and  070h
            rrc  a               ;4 byte offset into jtable
            ld   hl,jtable
            add  a,l
            ld   l,a
            jp   (hl)

`jtable` is the eprom's jump table — register, map, ram, fpp and bus
tests — and `setup`'s memory check ends in `tstsw`, the switch
dispatch proper:

    tstsw:  ld   a,(switch)
            and  0f8h            ;switch 4 down is 10h
            ld   d,a
            ld   e,0
            cp   0               ;all on: boot the hard disk
            jp   z,boothd
            cp   08h             ;switch 5 off: boot the dma controller
            jp   z,nuboot
            cp   10h             ;switch 4 off: boot the dj-dma floppy
            jr   z,djdma

`djdma:` is the host half of the handshake, in a one-minute
retry loop:

    djdma:  ld   h,10h           ;wait byte for 1 minute
    djlop0: ld   bc,0000h
    djloop: ld   a,(djstat)      ;read the status back
            cp   040h
            ld   de,(djstat - 2) ;d & e point to cold boot loader
            jr   z,check         ;if good status continue set gobuff
            cp   0ffh            ; - else loop for good status
            jr   z,nstat         ;if 0ffh then force to a zero
            dec  bc
            ...
    nstat:  xor  a
            ld   (djstat),a      ;null status byte ... signal DJ-DMA

`djstat equ 104ah ;adjusted channel address of status byte`, and 1048h
is the operand of the `JP 80H` in the stub.  Both are bus addresses
reached through the alias window: task 0's segment 1 points at bus page
0, so the monitor reads bus 004Ah at 104Ah.  It has to: in task 0 an
address below 1000h is the CPU board's own page — local ram at 0000h,
its registers at 0400h, the map at 0600h, the eprom at 0800h — and is
*not on the S-100 bus at all*.  The card's writes to bus 0000h-0037h
never touched the monitor's zero page; that they do not is why the card
can zero main memory under a running monitor.

`check` is where the condition is fixed exactly:

    check:  ld   a,(switch)
            bit  2,a             ;the monitor switch
            ld   a,1             ;normal task number
            ld   (u.pc),de       ;initialize the pc save area
            ld   (ctask),a
            jr   z,montor        ;jump if monitor desired
            ld   (mapram + 2),a
            ld   (map + 2),a     ;restore the maps
            jr   nutask

and `nutask` writes the stub and lets `setup` fall out into the pic
setup and `jp gobuff`:

    nutask: ld   hl,gobuff       ;Write a routine to switch to new task
            ld   (hl),03eh       ;- because when the task register is
            inc  hl              ;- written into, the lower half of the
            ld   (hl),a          ;- prom goes away.
            inc  hl
            ld   (hl),032h
            inc  hl
            ld   (hl),02h
            inc  hl
            ld   (hl),04h
            inc  hl
            ld   (hl),nop        ;6 nops for countdown sequence
            ...                  ;x6
            ld   (hl),0c3h       ;the jump op code
            inc  hl
            ld   (hl),e
            inc  hl
            ld   (hl),d

The stub is `3E 01 32 02 04 00 00 00 00 00 00 C3 80 00`: `ld a,1`, `ld
(402h),a`, six nops, `jp 0080h`.  It is in ram, not the eprom, because
the task register write takes the eprom's lower half away — the
monitor's own comment says so — and it spends six nops on the hardware
exchange, which lands on a later instruction fetch (mon500's own bank
routine spends seven).  The write to 0402h selects task 1 and, in the
same byte, the bank: the low nibble is the task whose page table is
used, the high nibble goes straight out to A20-A23.

## What the medium's first instruction sees

    task register 0402h   01h    task 1, bank 0
    mask register 0403h   1Ah    the monitor's hstrap: "allow traps on
                                 halts and stops, interrupts"
    ctask 0006h           01h    so the trap handler knows a trap came
                                 from task 1 and not from the monitor
    cmask 0007h           1Ah
    u.pc 000Ch            0080h  the boot address, from stub+0Ch
    u.de 000Eh            0080h  the same value, on its way to the stub
    A                     01h    the task number, as the stub set it
    DE                    0080h
    HL                    gobuff+0Ch, the scratch the stub wrote
    PC                    0080h  fetched after the exchange
    SP                    the monitor's own stack, near 0200h
    bus 0000h-0037h       the host's bytes, restored by the card
    bus 0038h-004Ah       the 19-byte handshake, flag 0FFh, jp 80h
    bus 0080h..           the medium's first sector

This is the stock condition — the one the media were written against.
Level 1 reproduces the first five rows, the PC and the last: task 1,
the identity map, 0080h, one sector.  The two memory rows are the
card's own manners, and nothing that boots from a distro diskette
looks at them.

Every one of those is a contract with code written in 1982:

  * **The load address is 0080h.**  Not 0100h, not the label's idea of
    anything.  A medium with a label is read by *our* level 1, which
    chooses; a medium without one was written against the card, which
    chose 0080h.
  * **SP is meaningless.**  The stub does not touch it, and the
    monitor's stack is in on-board ram that does not exist in task 1 —
    the same address is bus memory there, with the boot sector and the
    channel program in it.  A legacy boot sector must set its own SP
    before it pushes.  `1010-8_stand-alone`'s first instruction is
    `ld sp,3080h`.
  * **One sector, at the drive's own length.**  A reader must not
    assume 512, and must not read four sectors, before it knows what
    the medium is.  The card's length for an eight inch drive is 128;
    for a five inch drive it is the drive's own.
  * **Traps are live.**  hstrap in the mask register means a halt or a
    stop in the medium's code traps to the monitor, which re-enters
    with u.pc still pointing at the boot sector.  A medium that fails
    by halting is a medium the monitor gets back.
  * **No drive number is handed over.**  The card tried drives until
    one answered, and the sector has no way to know which one it was.
    Level 2 recovers it from the channel program's own drive byte,
    which the card leaves in memory at 0057h — see `djboot1.s`.

## The map at that moment

Task 0's page table, as `reset1` built it and `check` restored it:
segment 0 to page 0 ... segment 15 to page 15, with access attribute
3, except that segment 1 is left pointing at bus page 0 — the alias
window at 1000h-1FFFh — by `setup`, which does it on purpose ("a
window for DMA device commands").  Task 1's is the same identity
mapping, built by `reslp1` (`LD A,10h / LD B,0 ... cp 20h`: task 1,
segments 0-15 to pages 0-15), and `fmap` copies that pattern into tasks
2-15.

Address translation is the map's, not the medium's business, but it is
what makes 0080h mean what it means:

    pte = ((task & 0fh) << 5) + (page << 1)
    phys = ((task & 0f0h) << 16) | (mapram[pte] << 12) | (addr & 0fffh)

The map ram at 0600h is write-only; the monitor keeps its own image of
it at 0200h (`map:`) and writes both together, which is why every
`ld (mapram + n),a` in mon447 is followed by `ld (map + n),a`.  Task 0
can write any task's map ram, and only task 0 can — the map and the
task register are the supervisor's page.

## What hwsim changed, and the `-j` strap

`d1` used to emulate the card's bootstrap, so `-B djdma` and
`-c 0x714` booted a legacy diskette with the stock monitor and no level
1 rom at all.  Commit 55a9cca9 (2026-10-15) removed it: the low-memory
save and zero, the 19-byte stub at 0038h, the block-0 read to 0080h,
the status at 004Ah, and the five-inch renumbering are gone.  Nothing
replaced them, so `-B djdma` spun into the monitor's own `'F'` error
path ("controller not responding") — a contract change in the harness,
not in the machine, since `mon447.s` and `mon375.s` still depend on it.

The bootstrap is back, and it is a strap rather than a default: on the
real card J2 selects it, A to B is the non-bootstrapping position (which
is how our machine is wired, per `stand/boot/README`), and B to C is the
conditional bootstrap the stock monitor's `djdma` path needs.  So:

	d1 -b mon447.bin -B djdma -j bc <disk>

`-j bc` runs the nineteen bytes at 0038h, reads one sector of cylinder 0
side 0 to 0080h, and raises 40h at 004Ah — the handshake `mon447`'s
`djdma` polls.  `-j ab` (the default) does none of it, and that is the
right default because the level 1 rom is what our machines have and the
condition the card set is reproducible in a page of rom: level 1 is the
supported path, `-j bc` is the harness being the *stock* machine.

The configuration we run, and d1's default since 2026-10-15, is mon500
with the level 1 rom and no djdma boot anywhere in it:

	d1 <disk>		# -b mon500.bin -M l1boot.bin, J2 ab

`-b` defaults to `mon500.bin` and `-M` to `l1boot.bin`, both looked for
beside the tree's `stand/roms` first (`hwsim.c:1805-1835`), and the
switch register comes up at S6 off — which the negated switch reads as
set, meaning no monitor and a boot, and to mon500 a boot *is* the level
1 rom (`mon500.s:443-452`; nothing else on the switch is read).  So a
bare disk argument is a disk level 1 can boot, and level 1 is what walks
the drives and finds it.  The `-j bc` recipe above is the stock machine
reproduced — kept because `mon447.s` and `mon375.s` still lean on the
card, not because anything we run needs it — and `-b mon447.bin` is how
to get back to it.

Nothing on that line says which switch position it is, and nothing has
to: `-B djdma` *is* 14h, and 14h is what the emulator comes up with
(`switchreg = SW_DJDMA | SW_NOMON`, `mpz80.c:1543`), so the recipe above
runs as written.  A `-c` after `-B` wins, and one that leaves bit 2
clear is the trap worth naming, because the failure does not look like a
switch setting: `-c 10h` still selects `djdma` at `tstsw`, the card
answers, the boot address is read out of 0048h, `check` sets `u.pc` to
0080h — and then `bit 2,a` at `mon447.s:553` sends the machine to
`montor` instead, which prints the monitor's `FFFF` and its `:` prompt.
The card worked and the switch said not to boot.  14h is SW_DJDMA with
S6 high; bit 2 clear is "monitor", in mon447 and in mon500 both.

One deliberate imprecision: the card *hunts* drives, and `djdma_boot()`
reads logical drive 0 instead.  Renumbering the logical drives the way
the card does would change the guest-visible numbering that `djdma.4`'s
minors and the `-d`/`-5` names assume, so `-j bc` places the block the
card would place without moving the drive it came from.  Level 2
recovers the drive from the channel program's drive byte at bus 0057h
either way.

## What level 1 does

`stand/roms/l1boot.s` walks the rows twice.  Pass 1 (`rowatch`) reads
block 0 of every drive, stages it in this rom's own page, and looks for
the label; a block with no label in it is remembered as a legacy
candidate (`djprobe`, `djleg`) and a label always wins.  Pass 2
(`bootit`) re-reads the chosen drive through the window — task 0's
segment 1 points at bus page 0, so `win + 0100h` is bus 0100h, which is
where task 1 will run it — and enters it with `switch`, which writes
the same stub shape as `nutask`: `ld a,1`, `ld (402h),a`, six nops,
`jp <dest>`, and sets `ctask` first so the trap handler knows the trap
came from task 1.

The legacy difference lives in `djread`: when `(legac)` is set and this
is pass 2, the block is re-read as the one sector it is (`djfirst`),
copied to `win + 80h` instead of `win + 100h`, and entered at 0080h.

The numbering is the other half of that condition, and it is not in the
machine's state when level 1 runs.  `SET LOGICAL DRIVE` is the only thing
that moves it, the card does not renumber at power-on (J2 A to B), and a
medium's own boot sector asks for *logical* drive 0 — the drive it came
from — because the bootstrap it was written for runs LOGCAL
(`DJ49.MAC:898`) and makes the port that answered logical 0 to 3.  With
the card at its default numbering, logical 0 on a five inch medium is the
eight inch port, and the medium's `read track` comes back 82h, S_UNREADY.
So `djrenum` issues the same command the card's bootstrap issues — 02eh,
parameter 04h off the five inch port and 00h off the eight inch one — at
the default channel address 0050h, with the halt that ends the walk
written in after it, before the sector is entered.  It is the same
channel-program-plus-poll shape as every other read, and the media that
need it program the channel themselves: `disks/new/Micronix1.6/
stand-alone.IMD` writes `branch 00beh` at 0050h and runs its own program
from there, which asks for logical drive 0 and nothing else.

A *labelled* medium is not renumbered.  Level 2 and the kernel own the
numbering from there, and the kernel sets what its root device's minor
says — the 1.6 stand-alone reports `root dev: djdma/12`, minor 12 being
the five inch drive.

What level 1 does *not* reproduce, and need not:

  * the save, zero and restore of bus 0000h-0037h.  That exists for a
    host whose zero page is CP/M's, and nothing is running on our
    machine at that point; the medium's own code does not look.
  * the handshake at 0038h-004Ah, for the same reason: no monitor is
    polling, and level 1 is what boots instead of one.
  * the drive hunt.  Level 1 asks the user, having enumerated what is
    there, which is strictly more information than the card had.

What it must get right is the short list above: 0080h, its own SP, one
sector at the drive's length, task 1 with the identity map.  The
`dest` the row leaves behind is the *entry* address, so a legacy block
has to leave 0080h there and not only copy to it — `bootit` takes the
entry from `(dest)` and masks the high byte into a task-1 offset, and
nothing else in the path knows which kind of block it is carrying.

It must also leave the monitor's *image* of the map in step, and this is
the one thing about it that is not obvious.  The monitor writes 0600h and
0200h as a pair — `check`'s "restore the maps" above, and the copy loop
that puts this rom in place — because the map ram cannot be read back and
0200h is the only copy.  The kernel is the second reader: `sys/mem.s`
declares `IMAGE0 = 0x0200` as "readable version of MAP0" and every window
swap pops a page out of it and writes it to the map ram, so a byte left
stale there is a page the kernel will put back later.

Level 1 wrote only the map ram.  What it leaves in 0200h is whatever the
monitor's copy loop put there to stage this rom — page 0f0h in segment 1
and page 00fh in segment 2 — so the kernel's first overlay window swap
restored page 00fh into task 0 segment 2, returned into what is now this
rom's page at 0ff000h, and ran the rom's code until it wandered off.  It
presents as a boot that reaches the kernel's banner, prints its root
device, and then hangs with no message at all.  Every `ld (mapram + n),a`
in `l1boot.s` now has its `ld (map + n),a` beside it at all six sites.

## A worked example

`disks/dist/1010-8_stand-alone.IMD`, a Micronix 1.3 stand-alone
diskette.  Its cylinder 0, side 0 is FM, 128-byte sectors, ids 1-26,
and there is no MWDL anywhere in the first 256 bytes.  Loaded at 0080h
its first sector disassembles as:

    0080  31 80 30        ld   sp,3080h
    0083  3e 26           ld   a,26h
    0085  32 50 00        ld   (0050h),a   ;branch in channel
    0088  af              xor  a
    0089  32 53 00        ld   (0053h),a   ;extended address 00
    008c  21 dd 00        ld   hl,00ddh    ;the channel program
    008f  11 10 00        ld   de,0010h
    0092  cd c0 00        call 00c0h       ;run it, poll hl+de
    0095  23              inc  hl          ;-> 00eeh
    0096  11 06 00        ld   de,0006h
    0099  cd c0 00        call 00c0h       ;run that one too
    009c  3a e9 00        ld   a,(00e9h)   ;the sense reply's
    009f  07              rlca             ; sector length code
    00a0  5f              ld   e,a
    00a1  21 f5 00        ld   hl,00f5h    ;a table of
    00a4  19              add  hl,de       ; length -> offset,count
    00a5  7e              ld   a,(hl)
    00a6  32 90 30        ld   (3090h),a   ;patch the loaded stage:
    00a9  23              inc  hl
    00aa  46              ld   b,(hl)
    00ab  90              sub  b
    00ac  5f              ld   e,a
    00ad  21 c9 30        ld   hl,30c9h
    00b0  19              add  hl,de
    00b1  23              inc  hl
    00b2  05              dec  b
    00b3  ca bb 00        jp   z,00bbh
    00b6  36 ff           ld   (hl),0ffh
    00b8  c3 b1 00        jp   00b1h
    00bb  36 0a           ld   (hl),0ah
    00be  c3 80 30        jp   3080h

and the helper at 00C0h (which is why DE is an offset from HL there):

    00c0  22 51 00        ld   (0051h),hl  ;the channel address
    00c3  19              add  hl,de
    00c4  0e 0a           ld   c,0ah      ;10 tries
    00c6  af              xor  a
    00c7  77              ld   (hl),a      ;clear the status byte
    00c8  d3 ef           out  (0efh),a   ;run the program
    00ca  b6              or   (hl)       ;poll it
    00cb  ca ca 00        jp   z,00cah
    00ce  2b / 2b / 7e    dec hl/dec hl/ld a,(hl)
    00d3  fe 40           cp   40h        ;the command's status
    00d5  c8              ret  z
    00d6  0d / c2 c6 00    dec c / jp nz,00c6h
    00da  c3 da 00        jp   00dah      ;hang

The two channel programs it points at are in the same sector, and they
are ordinary DJ-DMA command strings — `setdma` (23h), `sread` (20h),
`sense` (22h), `halt` (25h):

    00dd  23 80 30 00     setdma 003080h
    00e1  20 01 01 00 00   sread cylinder 1, sector 1, drive 0
    00e6  22 00 00 00 00   sense drive 0 (chars, sector length,
                          drive status), then wait for 40h at 00eb
    00ec  25 00           halt, its status at 00ed
    00ee  20 00 03 00 00   sread cylinder 0, sector 3, drive 0
    00f3  25 00           halt, its status at 00f4

So: ask the card what the drive is, patch a byte and a couple of
table marks in the sector that the second read lands at 3080h, jump to
it.  Cylinder 0 sector 3 is where the real stage lives; the first read
of cylinder 1 sector 1 is discarded over by the second.  For a
128-byte medium the marks land inside 3080h-30FFh, which is exactly one
FM sector — the length the card read for it, and the length the code
patched for.

Three things this example proves about the medium's assumptions:

  * it sets SP itself, first instruction, because nothing else will;
  * it hands the card *bus* addresses (0050h, 00DDh) and expects the
    dma to land at 3080h — a flat 64k address space with no paging in
    it, which is what task 1 gives it and task 0 does not;
  * it reads the sector length out of the controller's sense reply
    rather than assuming one, which is how Morrow's own boot code
    survived shipping the same sector 0 on 8" and 5 1/4" media.

## Checking a change against this

  * A legacy medium is entered at **0080h in task 1**, with task 1
    identity-mapped, and with its own SP set by its own first
    instruction.  Anything that enters it at 0100h runs the wrong
    bytes.
  * The map ram and the monitor's image of it at **0200h are one
    object**: every write to 0600h+n needs its twin at 0200h+n.  The
    kernel reads `IMAGE0` as its own window table, so an image left
    holding a page the boot staged is a page the kernel restores.  The
    symptom is a kernel that loads, prints, and then hangs wordlessly.
  * The read before it is **one sector at the drive's own sector
    length** — 128 on an eight inch drive, the drive's own on a five
    inch one — from cylinder 0, side 0, the medium's lowest sector
    number.  Four sectors make a *label* candidate; one makes the
    program.
  * No label means the medium is a program: **never entered in pass 1**,
    where there is no task 1 to enter, and never entered at a length or
    an address the card would not have used.
  * A block with no label that is *also* not a program — an unwritten
    track's fill — is neither: `djcpm` refuses a block whose first 128
    bytes are all the same value, and the row reports the drive as
    answering but not bootable.
  * `-B djdma` alone no longer boots a legacy medium — the emulated
    bootstrap was removed in 55a9cca9 — and `-j bc` is what puts it
    back, because the real card only bootstraps with its J2 strap in
    that position.  The supported path is the level 1 rom, and it is
    the default one: `d1 <disk>`, which is mon500 and l1boot, with the
    card's J2 at A-B and no `-j` said at all.  The monitor matters,
    because only mon500 has the level 0 half that copies the socket rom
    down (`mon500.s:565-636`, `tstsw`'s `sw6` test at `:450`).  mon447
    has no copy of the 4k and no `f000h` anywhere in it: its `nuboot`
    and `boothd` read a block and enter it themselves, which is the
    labeled medium's contract and not the legacy one.  The two paths
    must agree about the condition and about 0080h; if a medium boots
    under one and not the other, the difference is the harness, not the
    medium.
  * Mon500's console is not free in d1, and it is that harness
    difference.  The monitor finds its own console and builds the driver
    for it: the Wunderbus 8250 at 048h-04fh or the SS1's 2651 at
    07ch-07fh (`mon500.s:753-790`).  The SS1's block is its jumper: it is
    070h-07fh here and not CompuPro's default 050h, because 050h-055h is
    the HDCA's and the HD-DMA's - two cards on one port is one card that
    is never heard again, and the SS1 is the one that used to win.
    d1 registers the 8250 only when the
    multIO is the interrupt controller and the 2651 only when `-w ss1`
    makes the SS1 one (`ss1.c:604-637`), so under the default wiring
    *neither* answers, and `findwb` is not at fault: its loopback probe
    reads open bus (0ffh) and correctly refuses.  `findss1` then fails
    on an unregistered port and `nocon` halts the machine, with nothing
    to say it on.  Boot mon500 with `-w ss1 -c 0x104` - the SS1 answers
    and 0100h gives the 2651 a terminal.  The level 2 chain writes its
    console to 048h-04fh whatever the monitor found (`tsk1:00e8` is the
    stub it installs), so under `-w ss1` its messages go nowhere; only
    the level 1 rom's own output reaches the console.
  * `-b` and `-M` default relative to the *current directory*
    (`hwsim.c:1805-1818`).  Run d1 from anywhere but the tree and the
    socket at 0ff0000h reads erased: mon500's copy loop fills 0f000h
    with 0ffh, `jp 0f000h` runs into it, and the `rst 38 at 0038` that
    follows is the emulator reporting a machine that ran off into
    nothing, not a medium that failed to boot.
