# Overlaying the drivers

A disk or character driver here can be built as a **module** instead of being
linked into the kernel: one 4K page holding its code and its private data,
linked at a fixed virtual address and copied at run time into one 4K segment of
the machine's memory. The kernel reaches it through the same `biosw[]`/`ciosw[]`
entries it always did, so nothing above a driver knows the difference.

`docs/overlay.md` is the general case; this is what this kernel does.

## The architecture

**The module.** A driver is linked on its own against the kernel's symbol table
at `OVLBASE`, cut to 4096 bytes, and appended to the kernel file past the
object's own extent. Its first bytes are a `struct ovlhdr` (`include/sys/ovl.h`),
which is the whole of what the kernel knows about it. The build makes four —
`dj` (floppy), `mw` (HD-DMA), `ide`, `ncr` (NCR 5380 SCSI) — and the list is
`MODS` in `sys/GNUmakefile`.

**The frame.** `OVLSEG` is one page of the kernel's own address space, the page
just past the end of the resident text, and `OVLBASE` is `OVLSEG * 0x1000`. The
final link is told the data segment starts one page above it (`-Tdata=OVLDATA`),
so the frame is the page between the text below it and the data above it, with
nothing linked into it: mapping that segment to a module disturbs no buffer
window, no scratch window and no pool. `OVLSEG` is a property of the build
rather than a choice — it moves when the resident text grows — so the kernel
link and every module link take it from one make variable and cannot disagree.

**The slot.** `ovlslot.o` (`sys/ovlslot.s`) declares one page of text at
`OVLBASE`. It is loaded by the loader that already runs, at the address it is
already linked at, and it exists so the file has bytes there for `setdev` to
overwrite. It is the page an installed kernel carries its root driver in.

**Placement.** One module is mapped into the frame at a time, and two things put
one there:

- `ovlstart()` (`sys/ovl.c`), called from `cus()` inside the board's existing
  `di()`/`ei()` window, places the driver `setdev -i` stamped into the slot. Its
  segment is `OVLSEG`, because that is where the page already is — the identity
  map the kernel runs under has it mapped, so "mapping" it is a register write
  that never changes value.
- `ovlplaceall()` (`sys/main_init.c`), at the end of the kernel's `main()`,
  places the rest. The loader leaves the inode it booted this kernel from in
  `kino` (`sys/uhdr.s`); the pass reads the 16-byte object header at the front
  of that file, derives the module region's offset from it, and for each whole
  page past that offset: read the first block, take the major out of it, skip
  the page if that major is already placed, else `segalloc()` a segment, copy
  the eight blocks into it through the frame itself (`putblk`), and
  `ovlattach(seg)`. A page that will not read, or is short, is a `pr()` and the
  next page; nothing panics.

`ovlplaceall` is init-only code, folded into `highmem.o` and reclaimed as buffer
headers once it has run, so it costs no resident space. It runs before the
swapper forks, so nothing else is in flight while the frame is spoken for.

**Resident and module are one mechanism seen twice.** A driver the kernel still
holds has a header that is a kernel object, a segment of 0, and needs nothing
mapped; a module has a header at `OVLBASE` and a segment of its own. `ovlmap`
returns the right one either way, and the tables below never distinguish them.

## The invariants

**Every driver has the same virtual address.** A module is linked at `OVLBASE`,
so a function or a static object inside one is `OVLBASE + offset` — and that is
true of every module. What makes an address mean something is the *mapping*, not
the number: one module is in the window at a time, and a module's addresses are
valid exactly while that module is the mapped one. Everything below follows from
this one.

**A module's page is a physical segment it holds for the whole boot.** The map
is a window onto it, and the bytes never move. So restoring the window restores
the bytes, and **an overlaid function may sleep** — the map is restored on the
way back and the page is still the driver's. This is the difference between this
design and one that swaps contents.

**Everything a driver owns lives in that page.** A driver runs with its own page
in place whichever door it came in through, so its queue state, its private data
and its read-only tables can all be in the module — and the cost is that the
*whole object* has to fit 4096 bytes.

**The window is restored per process, by the scheduler.** `u.drv` holds the
segment of the page the process is running in. `ovlmap` and `ovlcall` write it
*before* the map register, and `ovlremap` — called from `newmap()` at every
switch, with the u page already the incoming process's — writes it back as it
stands. That is what makes a process that slept inside a driver resume in its
own page however many other drivers ran while it was out. A process not inside a
driver has `u.drv` 0 and is left alone; a stale page costs nothing because no
resident code reads `OVLBASE`.

**Every door maps first.** There are four ways into a driver and each one has to
put the module's page in place before it calls:

- The trampolines. `biosw[]` and `ciosw[]` point at `ovlopen`/`ovlclose`/
  `ovlstrat`/`ovlioctl` and
  `ovlcopen`/`ovlcclose`/`ovlcread`/`ovlcwrite`/`ovlcmode`. Each
  recovers its major from the device it was handed (`bmajor`/`cmajor`), maps,
  and calls the entry the header registered.
- An interrupt line. `ovlint[line * OVLSHARE + i]` holds a `{fn, seg}` pair, and
  `ovlntr` calls each through `ovlcall`. An interrupt arrives with any module
  mapped or none, so it is the *line*, not the interrupted process, that decides
  what is mapped.
- The tick. `ovltickc[maj]` × `ovlseg[maj]`, through `ovlcall`.
- The DMA bus lock's heir. `busmaster`/`busmasterseg`/`busheir`/`busheirseg`
  live in `sys/ovl.c` and `busgive` calls the heir through `ovlcall`. This is a
  driver calling a driver, with neither a trampoline nor a line segment in
  sight, which is why it needs saying separately.

**The table entry, not the segment, says whether a driver is there.** A driver
the kernel still holds has entries and no segment. So the trampolines test
`ovlbvec[maj].open`, and `ovlmap`'s answer is not a test.

**The frame costs nothing.** It is the page between the end of the resident text
and the start of the data, placed where nothing else can be. Init-only code is
not text for this purpose: `highmem.o` is folded to data and parked after bss, so
it is above the frame and is reclaimed as buffer headers rather than counted as
something the frame must clear.

**The tables a placement writes are the kernel's data, never the u page.**
`sys/consts.c` links into `upage.o`, which every process has its own copy of and
which fork's `segcopy` clones; `sys/ovl.c` does not. The same rule puts the bus
lock's words in `ovl.c` rather than beside the leaf code in `sys/bus.c`: a
transfer's `busget` and its `busgive` are not inside one process, because both
drivers sleep holding the bus, so in the u page a `busgive` in another process
would read its own heir, find 0, and drop the one the sleeping process
registered.

**A page whose major is already placed is skipped.** Placing it again would put
a second handler on the interrupt line — every interrupt served twice, the second
pass draining a chip the first has already drained — arm a second tick over the
same driver, and move the driver to a segment of its own while the running copy
stays where it was. `ovlmap(maj) != 0` is the test, and it answers for a driver
the kernel still holds as well as for one in a segment.

## The interface

`struct ovlhdr` (`include/sys/ovl.h`) is a module's first byte, at `OVLBASE`. It
is the whole of the interface in both directions:

```c
struct ovlhdr {
    UINT major;                 /* the major this module serves */
    int (*init) ();             /* init(seg), once, before it is reachable */
    int (*tick) ();             /* tick(), once a second, 0 for none */
    char *data;                 /* the driver's data, in its own segment */
    struct biovec *bvec;        /* its block entries, 0 for none */
    struct ciovec *cvec;        /* its character entries, 0 for none */
    UINT line;                  /* the line intr is on, if there is one */
    int (*intr) ();             /* the interrupt handler, 0 for none */
    char name[12];              /* what devname[] reports for the major */
};
```

**`major`** is which major the module serves, and it is how the module is found:
the page declares it and `ovlplace` registers it under that. So the appended
pages may sit in any order, a module can be added or dropped without a table to
keep in step, and there is no index to break quietly. `OVLMAJ` is 16; a major of
0, one at or past `OVLMAJ`, or one past both `nbdev` and `ncdev` is refused.

**`init(seg)`** is called once, with the page in place and nothing able to reach
the driver yet. It is for the hardware alone — the controller's timers, the DMA
channel, the interrupt enable — and **what it returns decides whether anything
else happens**: non-zero is a driver whose hardware is not on this machine, and
such a driver is not registered at all. Its major goes on answering the way
`nodev` does and the only thing spent is the page. There is nothing partial to
unwind, because none of the rest of the header has been read yet. That is what
lets one kernel carry a driver for a card that is not plugged in.

Everything after init comes from the header in this order: `tickset`, `nameset`,
`bvecset`, `cvecset`, `intrset`. Init is hardware only and cannot be the thing
that names the driver's entries.

**`data`** is where the driver's data structure lives, and it is the one thing
init cannot work out for itself: a resident init is kernel code and cannot name
anything in a module — not a function, and not a data object either, since
knowing the segment gives it a page and not an offset into one. So the module
says where its data is here, at a link-time constant, and init reaches it through
`ovldata(major)` and no other pointer. An init that wants a second data object
wants it inside this structure. The structure is opaque to the kernel.

**`bvec` and `cvec`** are the entries the kernel will call through. A block
driver fills `bvec` (four entries: open, close, strategy, ioctl), a character
driver `cvec` (five). A module that is both fills both, but no module in the
tree is — every one is a block driver, and `dj`'s command interface is
`djioctl` through `bvec`, not a `cvec`; its header leaves the `cvec` field `0`.
The kernel copies the entries into its own tables at placement, because a function
that lives in a module is only a function while that module is the one mapped.

**`line` and `intr`** name the interrupt line and the handler. A line carries
`OVLSHARE` (2) handlers, end to end in a flat `ovlint[NOVLINE * OVLSHARE]` array
(`NOVLINE` is 3), and the claim goes in the first free slot; a full line is
refused the way a bad one is. `ovlntr` calls **every** handler the line carries,
in the order they were placed, rather than stopping at the first taker: the cards
on one line share it open-collector and the only thing that clears a card's
request is that card's own status read, so no card can speak for another. VI2
carries `ide` and `ncr`.

**`tick`** is the same problem as `intr` arriving by a different door: a
controller that has stopped talking raises nothing, and a driver that wants to
notice has to go and look on a clock of its own. One resident clock
(`ovltick`) serves the whole system, walked across `ovltickc[]` and re-arming
itself at `TICKINT` (= `HERTZ`); `ovlarm` starts it once, the first time a driver
asks for one. A driver that wants a longer period counts its own turns (`dj`'s
`DJTICKS` is 10). The kernel's `tlist[]` holds five timeouts in total and a
driver's watchdog is never stopped, so a timer per driver is not available.

**`name`** is what `devname[]` and so `pcon()` report for the major. It is a list
of characters and not a string, and that is not a style: `ccc` emits a copy of a
string literal ahead of the object's data, so `"djdma"` would put six dead bytes
in front of the header and it would no longer start at `OVLBASE`. A header
object holds the struct and nothing else, for the same reason.

**Nothing calls in.** A driver declares itself in its header; it does not
register itself, and `bvecset`, `cvecset`, `intrset`, `nameset` and `tickset` are
`static` in `sys/ovl.c` for that reason.

**A major with no entry behind it answers the way `nodev` does.** For a block
major, `ovlstrat` sets `BERROR` and `ENXIO` and calls `iodone`. So a disk that is
not loaded is indistinguishable from one that was never there — except that a
request against it completes with an error instead of hanging, which is what a
major that may be loaded later has to do.

**A driver reaching the kernel is an ordinary call.** It calls `bhold`, `iodone`,
`sleep`, `di`, `copy`, and those are relocations against symbols a page linked on
its own cannot see. `mxld -A<image>` is the answer: *resolve undefined symbols
from a linked image*. A module is linked `-s -Sdata -Tdata=$(OVLBASE)
-Aunix.link` — the kernel having been linked first — and every kernel entry it
names is filled in with the real address out of the kernel's own symbol table.
There is no vector table and no second description of the kernel's interface to
fall out of step with the first. The kernel must be linked before any module is,
which is the direction the append-to-the-kernel-file arrangement already runs.

### The build

`sys/GNUmakefile`:

- `$(MODS:%=%.mod)`: `mxld -s -Sdata -Tdata=$(OVLBASE) -Aunix.link -o $*.mod
  $*hdr.o $*.o`. The header is an object of its own, named **first**, and
  `-Sdata` folds every section into the data segment, where an object's data is
  placed in the order the objects are named — so the header's bytes are the
  page's first bytes and nothing the driver holds can get in front of them.
  `-s` drops the symbol table: a module is linked against and never linked from.
- `$(MODS:%=%.page)`: refuse a `.mod` larger than `16 + 4096`, then
  `dd bs=1 skip=16 count=4096` and `truncate -s 4096`. What the link produced is
  a 16-byte object header followed by the page's bytes, all of them.
- `unix`: copy `unix.link`, pad to a page boundary, `cat` the pages on, then
  `setdev -i`. The pages go on a *copy* rather than onto `unix.link` so a second
  make cannot stack a second set of pages on the first.

`setdev` (`cmd/setdev/setdev.c`) is the installer's tool: `setdev [-i] <kernel>
[<rootdev> <swapdev>]`. With no device arguments it prints what the kernel says;
with two it sets the device numbers; `-i` is the install mode, which stamps the
root driver and writes no device numbers. It reads the slot's address out of the
symbol table (`_ovlslot`; hence "no symbol table - do not strip the kernel"),
derives the module region as `16 + text + data + symbol table` rounded up to a
page, and finds the page whose header serves the major `_rootdev` already names.
Driver and device numbers are written in **one pass**, so they cannot disagree,
and `-i` writes no device numbers because the default in `sys/main.c` is the only
place they are stated.

The symbol table is in that extent and is the term that is easy to drop: leaving
it out would put the module region inside the table. `setdev` computes it and
`ovlplaceall` recomputes it from the object header, so it is not a number anybody
keeps in step by hand.

### What a minor means

`dev/devlist` is the authority. `major(dev) = dev >> 8`, `minor(dev) = dev &
0377`, `devslice(dev) = (minor >> 5) & 7`, `devtype(dev) = (minor >> 2) & 7`.

`mw`'s nodes encode the drive in bits 0-1 and the drive *type* in bits 2-4, so
`m16a`..`m16d` are minors 8..11. `ide` and `ncr` have one board, so their type
bits are zero and the minor is a plain unit number — a SCSI target id, for
`ncr`. The slice is bits 5-7 for all of them; slice `c` (bit 6) is the
whole-disk view, the drive at cylinder 0 with no roll, whose block 0 is the
label.

## TODO

Technical debt, in the order it is likely to bite.

**Address identity across modules.** Every module is linked at `OVLBASE`, so a
function pointer or an event address is `OVLBASE + offset` and is not unique to a
module. Two places where that matters, both currently lucky rather than correct:

- `busget`'s `busmaster != func` test (`sys/bus.c`). Two modules whose bus
  functions land at the same offset would compare equal, and the second would be
  told it already holds the bus.
- `sleep`/`wakeup` match on the event address alone. Events in use today do not
  collide, but nothing checks that, and `dj`'s `busplease` is the unguarded shape
  — `if (!busget(busready)) sleep(busready, DJPRIORITY)`, not a loop re-testing
  the condition — so one false wakeup would drive the bus without holding it.
  (`mw`'s heir `mwstart` does not sleep, so it is safe by construction.)

**A segment leaked when init refuses.** `ovlplaceall` takes a segment with
`segalloc()` before it calls `ovlattach`, and `ovlplace` clears the table entries
on a failed init but nobody calls `segfree`. So a driver for a card that is not
plugged in costs a segment for the rest of the boot, not just its page.

**The root driver can never be overlaid.** The stamped page is placed from the
slot and stays there; `ovlplaceall` skips its major. So the slot is one page of
the kernel's text spent permanently, and how much of it is wasted depends on
which driver the installation stamped: in the current build `mw` is 2683 bytes
and wastes 1413 of the 4096, while `dj` is the largest module at 4085 and leaves
only 11. It also means
there is no path for a boot device that is not the root device: the boot
device's driver would have to be read back out of the file, and reading the file
needs the root mounted.

**A failed interrupt claim is silent.** `intrset` returns -1 when the line is
full, and `ovlplace` ignores the return. A driver is then registered with its
entries and no interrupt, and has no way to notice. `OVLSHARE` is 2, so a third
card on one line is the case.

**`setdev` requires an unstripped kernel.** It reads `_ovlslot` from the symbol
table and dies otherwise. `unix.link` carries one; a stripped kernel cannot be
stamped at all.

**The module region is derived, not declared.** Position comes from the object
header and count from `i_size`; a trailing partial page, or a page appended out
of alignment, is silently not a module. `setdev` and `ovlplaceall` compute the
same number independently, and the only thing that catches a disagreement is
`setdev` dying when it finds no module for the root device.

**A one-board card has no type bits, and the guest `mkfs` believes them.**
`ide` and `ncr` nodes have `devtype` 0, which is the m5 row of the geometry
table `cmd/mkfs/mkfs.h` carries, so `mkfs` in the guest lays out a 5-meg
filesystem on a 16-meg disk with the boot area landing in the middle of file
space. Only `mw` nodes have ever been formatted this way by the tree's install
scripts, so the path has never been run in anger. The host `mkfs` takes the
geometry from the label instead, which is why the host-made images are correct.

That the guest `mkfs` still reads the minor at all is now the whole of the
exposure, because `sys/mw.c` no longer reads the type: the driver maps what the disk's
label says, and `mkfs` lays out what its own table says, so the two can disagree
where they used to be one table read twice. What keeps them together is that the
label was written by the same model the row names - `mwformat -m m16` and
`mkfs /dev/m16a` - which is the install scripts' habit and, now, load-bearing.
