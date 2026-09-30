# STUB.md — the stub driver, and how to make it real

`stub.c` and `stub_intr.c` are a block-device driver with every hook a
driver has and none of the code: each routine is a stub that returns
immediately. This file is the recipe for filling them in. Read it beside
`docs/INTERRUPTS.md` (how an interrupt reaches a handler) and `docs/overlay.md` /
`docs/DRIVERS.md` (what it means to be a driver module).

The two files:

- **`stub.c`** — the driver body: `open`, `close`, `strat`, `ioctl`,
  `init`, `tick`, the `struct biovec`, and the `struct ovlhdr` header.
- **`stub_intr.c`** — `stubint()`, the interrupt handler, in its own file
  so the header can name it without pulling the handler into the body.

---

## 1. The hooks

A block driver answers through four entries (the `struct biovec` in
`include/sys/con.h`), plus the three the header names (`init`, `tick`,
`intr`). Each is a stub here; what it has to become is below.

### `open(dev, mode)`

Called when something names the device — a mount, or an open of the raw
device. Probe the controller if that has not happened, reset it, take the
hardware. Return `0`, or an errno. `ncrinit.c` is the probe example; the
NCR driver does its reset here rather than at init so a machine that came
up with the bus in a strange state still gets a clean start.

### `close(dev)`

The last user is gone. Release the hardware and undo what `open` took.

### `strat(b)`

The whole of block I/O. `b` is a `struct buf` (`include/sys/buf.h`): the
flags, the device, `blk` (a filesystem block), `count`, and `data`. The
ordinary shape is one 512-byte sector — `strat()` fixes `count` at 512
before routing — but `swapio()` reaches a strategy routine directly with
a 4096-byte count, so a driver that moves only 512-byte sectors must
refuse the other shape rather than move one sector of it and call that
success (that is exactly what `idestrat` does).

Two ways to finish a request:

**Synchronous** (what `idestrat` does, `sys/ide.c`):

```c
if (b->blk > lastblk) { b->flags |= BERROR; b->error = ENXIO; iodone(b); return; }
if (b->count != 512)  { b->flags |= BERROR; b->error = EIO;   iodone(b); return; }

start the controller;
p = bhold(b);                 /* the buffer's bytes into the window */
move the sector in or out through p;
brel();
iodone(b);
```

`bhold`/`brel` (`sys/uio.c`) map the buffer's segment into the BUFSEG
window; the rules are that nothing sleeps between the hold and the
release, and nothing reaches a buffer header or maps the window for its
own use while the hold is up.

**Interrupt-driven** (what `mw.c` and `dj.c` do):

```c
start the controller;
while (!done)
    sleep(&done, PRIBIO);       /* stubint() wakeups this */
if (error) { b->flags |= BERROR; b->error = ...; }
iodone(b);
```

The request's done word lives in the driver's own `struct` (below), the
sleeper is `sleep(chan, PRIBIO)`, and the interrupt handler does
`done = 1; wakeup(&done);`. `PRIBIO` is the block-I/O priority.

### `ioctl(dev, cmd, r, b)`

Run the raw command block in `b` against the device (`include/sys/ioctl.h`).
A driver whose bus has no command block leaves the fourth biovec entry `0`
and answers `ENOTTY` here — the stub's answer. Every block driver in the tree
now fills the entry (`mwioctl`, `djioctl`, `ideioctl`, `ncrioctl`, in
`mwbvec`/`djbvec`/`idebvec`/`ncrbvec`); only the stub itself answers
`ENOTTY`.

### `init(seg)`

The one entry the header hands the kernel, run once at placement with the
module's page already mapped, before the driver is reachable. Probe for
the card and switch it on. Return `0` to register the driver, or non-zero
to say the hardware is not in this machine — the major then keeps
answering as `nodev`, which is how a kernel can carry a driver for a card
that is not plugged in. A resident init cannot name anything in the
module — not a function and not a data object — so it reaches the
driver's data only through `ovldata()` (`sys/ovl.h`). `ncrinit.c` is a
probe that needs no data; `djinit.c` is one that does.

### `tick()`

Once a second, page mapped, for a driver that must go and look at a
controller that has stopped talking (a timed-out request). Most drivers
leave the header's `tick` field `0`; if this one keeps `stubtick`, name it
in the header.

### `intr()` — `stub_intr.c`

The handler the header's `line`/`intr` fields name. The dispatch table
(`sys/intrpt.s`) CALLs the common wrapper (`sys/mio.s`), which saves the
registers, calls the handler, and writes the EOI — so the handler reads
the status, decides, sets the done word, `wakeup()`s, and returns. It does
not save registers and does not touch the 8259. It must not sleep and
must not reach a buffer header or the window. A real one first checks that
*this* card raised the shared line (two cards can share a VI line) and
returns untouched if it did not.

---

## 2. The driver's data

The `struct stub` at the top of `stub.c` is where a real driver keeps its
board state — the controller's register addresses, the request in flight,
the geometry it read off the card. The header's `data` field points at it,
because `init` and the entry points are the only doors and a resident init
cannot name a module's object except through that pointer. Fill the struct
out, and put its address in `stubhdr.data`. The kernel treats it as
opaque: it is an address, and what the driver keeps there is its business.

---

## 3. The header, and splitting it out

`stubhdr` (`struct ovlhdr`, `include/sys/ovl.h`) is the module's first
byte and the whole of the interface — the kernel holds no driver symbol.
The fields, and what to put in them:

| field | stub value | what it becomes |
|---|---|---|
| `major` | `0` | a free major; 0 is reserved for `nodev` |
| `init` | `&stubinit` | keep |
| `tick` | `0` | `&stubtick`, or `0` for none |
| `data` | `0` | `&stubdata`, the driver's struct |
| `bvec` | `&stubbvec` | keep |
| `cvec` | `0` | a char switch, if the driver is also a character device |
| `line` | `0` | the interrupt line (below) |
| `intr` | `&stubint` | keep |
| `name` | `"stub"` | what `devname[]` and `pcon()` report |

**The header belongs in its own object, named first on the link line.**
A module is linked `-Sdata`, which folds everything into the data
segment, and an object's data lands in the order the objects are named —
so the header must be the only thing in the first object, or the bytes in
front of it push it off `OVLBASE`. `idehdr.c` is the pattern; when this
stops being a stub, split `stubhdr` into `stubhdr.c` and name it first.

---

## 4. The interrupt line

`line` is an 8259 input number (0–7), not a raw bus line. From
`docs/INTERRUPTS.md`:

| Input | Owner |
|---|---|
| 0 | hard disk (hdca / hddma) |
| 1 | djdma floppy |
| 2 | ide hard disk |
| 3–5 | the three 8250 ACEs |
| 6 | master parallel port |
| 7 | RTC clock tick |

Only 0, 1 and 2 reach the S-100 bus; 3–7 are the MultIO board's own
devices. A new card shares a bus line and needs its own `card` id in
`set_vi` (`hwsim/s100.c`); see how `hdca.c` and `hddma.c` both sit on
line 0 with different card ids. The handler is then reached through the
fixed `intrpt.s` table — level `n` lands in `intN` — so a new line means a
new dispatch slot and, if it is a bus line, the simulator's `multio.c`
`reg_intbit`.

---

## 5. The build

`sys/GNUmakefile` builds each module from `MODS`:

```
MODS = dj mw ide ncr
$(MODS:%=%.mod): %.mod: %hdr.o %.o unix.link
	$(MXLD) -s -Sdata -Tdata=$(OVLBASE) -Aunix.link -o $@ $*hdr.o $*.o
```

To add this driver:

1. Split the header into `stubhdr.c` (section 3).
2. Add `stub` to `MODS`.
3. Pick the `major` and the interrupt `line`, and fill in the header.
4. The module is one 4K page; the build stops if it overflows. Everything
   the driver owns — code, queue state, private data, read-only tables —
   must fit that page.

---

## 6. Adding boot code to the multIO ROM

A new device that should be bootable also needs a boot driver in the
selector ROM, `stand/roms/multIO.s` (the 4K program at `0xff0000` that
MON5.0 copies down to `0xf000` and runs; `docs/BOOTROM.md` is its map). The
selector arms the IDE board and the NCR 5380 SCSI, races them, and reads
sector 0 from the winner into task 1's `0x100`. A third device plugs into
that same shape in three places.

The multIO ROM's structure: each controller has a **setup half** (arm it,
return) and a **boot half** (assume it is ready, read sector 0, switch to
task 1), and a shared poll loop watches one "ready" bit per controller:

```asm
entry:
	call idesetup
	call scsisetup
race:	call rdstat          ; IDE DRDY?
	and  40h
	jr   nz,idewin
	in   a,(scsistat)      ; SCSI BSY?
	and  scsi_bsy
	jr   nz,scsiwin
	... timeout -> no boot device
idewin: ... call ideboot
scsiwin: ... call ncrboot
```

To add the new device:

1. **`stubsetup`** — reset/select the controller and leave it armed, the
   way `idesetup` (reset the drive) and `scsisetup` (reset the bus and
   select target 0) do. It returns; it does not wait.
2. **`stubboot`** — assume the controller is ready: load its task file or
   send its command, read 512 bytes of sector 0 into the segment-1 window
   at `0x1100` (which is task 1's `0x100`), then fall into the shared
   task switch.
3. **The race** — add one arm to the poll loop that checks the new
   controller's "ready" bit between the IDE and SCSI arms, and a
   `stubwin` label that calls `stubboot`.

The task switch at the end is the existing `switch` routine: it writes the
`ld a,task / ld (task),a / nops / jp` stub into `gobuff` (`0x1b0`) and
runs it, because writing the task register drops the boot ROM.  Every boot
half ends `ld de,0100h / ld a,1 / jp switch` — the sector just read, run
as task 1.

Two things the boot code has to get right that the kernel driver does not:

- **The window.** The selector runs in task 0, where every address below
  `0x1000` is the CPU board's own RAM/PROM, so a store to `0x100` lands in
  the board's static RAM, not in memory task 1 will see. The boot halves
  write through task 0's segment 1 pointed at physical page 0, which makes
  `0x1100` in the supervisor the same byte as `0x100` in task 1. `ideboot`
  and `ncrboot` both do `xor a / ld (mapram+2),a` to open it.
- **Chatty output** goes through `uout` (Wunderbus group 1), and the
  strings at the end of the file are printed with `uputs`; a new device
  gets its own `strXXX` and prints `"xxx"` when it wins, exactly as the
  `ide`/`scsi` winners do.

Rebuild with `make` in `stand/roms` — the `multIO.bin` rule is `cp` the
`.cim` then `truncate -s 4096`, because there is no on-board data below
the code to skip.  The simulator serves it at `0xff0000` (`-M` overrides
the name, default `multIO.bin`), and MON5.0's switch-0 setting (`-c 0x04`)
is what reaches it.
