# Overlaying the drivers

Status: plan (not implemented). Follows `overlay.md`, which states the general
case; this is the concrete plan for *this* kernel, with the budget that makes it
possible and the two problems that are actually hard.

## What this is for

Every disk and character driver here is reached only through `biosw[]`/
`ciosw[]`, and at most one of them is in use at a time. So they can all share one
4K logical page: each driver is linked to that same address, its code and private
statics together, and "swapping a driver in" is one MMU register write rather
than a load from disk. Each driver's bytes live permanently in a physical
segment out of the 1M, so the remap never moves them.

The goal is that a driver fits **completely** in one page — not the top end only,
which is what `overlay.md` proposed when the budget could not cover the rest.
That changes the interrupt answer, and interrupts are where this plan gets
interesting.

## The budget, and why the dj.c change was the entry fee

Taking three drivers out of the 64K and putting one page back leaves the
difference free, and that difference is where the aligned page comes from. That
is the intent. What the measurements below say is that the *park* binds before
the page does, and that it does not fit yet.

`sys/dj.c` was the first instalment. Its 1K sector buffer `kbuf[1024]` was in the
kernel's own segment; it is now a `segalloc()`d page reached through the scratch
window (`SCRSEG`/`swin`/`srel`). Both revisions compiled, so this is measured
rather than remembered:

| | text | data | bss | total |
|---|---|---|---|---|
| `dj.o` before | 3001 | 1419 | 57 | 4477 |
| `dj.o` after  | 3264 |  396 | 64 | 3724 |

The object lost 753 bytes, and the kernel's own segments moved like this:

| | before | after | delta |
|---|---|---|---|
| text, from 0x1000 | 36095 | 36467 | +372 |
| data + bss        | 11865 | 10854 | -1011 |
| `_ebss`           | 0xcb58 | 0xc8d9 | **-639** |

The buffer that left is 1016 bytes. The code that reaches it is 372, of which 263
is `dj.c`'s own `swin`/`srel` call sites and 109 is `uio.c`'s new `swin`/`srel`
plus its 5 bytes of hold stack. **The reclaim is 639 bytes** — not the 1016 the
buffer's size suggests, and 639 is the number the overlay has to spend.

None of it became buffers. `MAXBUFS` (256, main.c) caps the pool either way, so
what changed is the gap between the last minted header and BUFWIN: 80 bytes
before, 719 after. It is growth room, not cache.

`mxnm unix | grep _ebss` is the way to re-check the end and `mxnm -b <obj>` the
segments — text and data only, so bss comes from `.ebss` minus `.edata`. The
numbers here went stale once already.

## The switch, and the one discipline that matters

Swapping in is one register write: `map0[2*OVLSEG] = image0[2*OVLSEG] = seg`.

The discipline is that **the mapping must be correct again by the time a sleeping
process resumes**, and that this has to be the *scheduler's* job, not the
driver's and not the trampoline's.

Kernel code is not preempted — a switch happens only across `sleep()` — so there
is exactly one place the overlay can be wrong, and restoring on the resume side
closes it. But a global "current overlay" that the trampoline writes on entry is
**not** sufficient: a process that sleeps inside a resident function and wakes
returns *into* the overlaid frame without passing through the trampoline again.
So the overlay segment has to be per-process state, written on the map switch
beside the u page's:

```c
malloc.c:309   map0[2 * USERSEG] = image0[2 * USERSEG] = n->mem[16].seg;
```

Give the process an overlay segment, write `map0[2 * OVLSEG]` on the same path,
and the restore is free — the driver never cooperates, and the trampoline only
has to swap in at entry. It also makes the overlay naturally per-process, so two
processes running two different drivers do not fight over the page.

Corollary: an overlaid function is still required to be *non-blocking* in the
sense `overlay.md` describes. Nothing in this plan lets an overlaid frame sleep
and expect to be resident when it wakes — the mapping is restored, but the
*bytes* are whatever process's overlay the scheduler put there.

**What is built takes the entry half of this and leaves this half open.** A
trampoline maps the major's segment at entry (`ovlmap`), and `ovlcall` saves and
restores `image0[2 * OVLSEG]` around an interrupt or a tick — which is correct
because neither can sleep. The per-process overlay segment this section describes
is not built: a driver whose `strat` sleeps can wake to another major's page. No
driver does that today, so nothing is broken today; it is the constraint a driver
must meet before it is moved out, and the reason the discipline above has to be
settled before the first one is.

## Interrupts are the wild card

An interrupt can arrive with any driver's page mapped, so an interrupt entry
cannot simply be an address inside an overlay: by the time the handler runs, the
bytes at that address may belong to a different driver. Hence **mapping
trampolines on the interrupt lines**.

`sys/intrpt.s` already has the right shape — one stub per line, each calling
`intrupt` with an inline handler address. Each becomes: swap in *this line's*
driver segment, then call `intrupt` with the entry address inside the overlay.
A shared line (`int2` is the IDE card and the SCSI adapter) swaps for each in
turn, or to a resident fan-out that swaps per card.

This works because `intrupt` runs with interrupts off for the handler's duration,
so the swap and the call cannot be interrupted between them, and nothing else can
remap the page while the handler is on it.

The alternative — keeping each driver's FSM resident, which is what `overlay.md`
preferred — is not available here, because the whole point is that the driver
fits *completely*. The price is paid instead in:

## The resident set

This section used to argue that part of a driver's state had to stay resident,
because a resident interrupt handler would otherwise chase pointers into a page
that now belongs to another driver. The design does not work that way, and the
measurement is different because of it.

An interrupt handler runs with **its own segment mapped**: the line carries the
segment its handler lives in, and the dispatcher puts that page in place before it
calls (`ovlntr`, `sys/ovl.c`), because an interrupt can land at any moment, with
any module mapped or none. The driver's tick is served the same way, and so is
every call the kernel makes into a driver — a trampoline maps the major's segment
before it calls the entry it registered. So a driver runs with its page in place
whichever door it came in through, and everything it owns can live in that page:
the queue state the handler walks (`dm[NDRIVES]`, `curbuf`, `qhead`, `qtail`), the
staging state it updates (`kdev`, `ksec`, `kw`, `kseg`), `djcomm[26]`, and the
read-only `specs[]`/`delay[]` tables `getstat` reaches through `d->specs` and
`d->step`/`d->settle`.

The last of those is the one that is easy to miss — 329 bytes of tables that look
like data and belong to the driver, read on the interrupt path (`specs[]` is 27
rows of 11 bytes). It lives in the page with everything else.

The cost is therefore that the **whole object** has to fit, not the object less a
resident set. Measured with `mxnm`: `dj.o` is 3789 bytes against the page's 4096,
307 spare; `mw.o` is 2749 and `ide.o` 2805. The page is not what binds — a 4K slot
is comfortable for all three — but it is the number to watch, per object and
measured rather than estimated.

## The link

Each driver is linked on its own at `OVLBASE`, producing a blob of no more than
4K, and the blobs are appended to the kernel image after the normal load pages.
They ride in with the kernel and are parked in the region after bss; **kernel
init code then `segalloc()`s a page per blob and copies each one into it**, and
the region they were parked in is handed to the pool as buffer headers.

That copy costs nothing, and that is the whole reason it is done there. Init-only
code already lives in that region and is already reclaimed: `highmem.o` is folded
data-only with `-Sdata`, parked after bss with `-Shigh`, and `expand_bufs()` mints
headers over it once init has returned. So the loader is not touched, and has no
budget of its own to stay inside — the copy is init code, and init code is free
because its address space comes back. The blobs are resident only for the length
of the boot, and so is the code that moves them.

The ordering is the one thing to get right: the copy has to finish before
`expand_bufs()` runs, because it is reading memory the pool is about to hand out.
`highmem.o` already lives under exactly that constraint — it runs *as* the region
it is about to be reclaimed from.

Two things this has to settle:

- **Who owns the segment numbers.** The pages are taken by init, so the kernel
  knows what it took — but `meminit()` sets `nsegs` and `segalloc()` sweeps from
  16 upward, so where the overlay pages are taken from decides whether anything
  else has to be told about them. Taking them from the top of the 1M, where the
  upward sweep never reaches, is the version with the least to go wrong.
- **The park has to fit.** The blobs are resident while init runs, so they have to
  fit in the headroom, which is what makes `sys/TODO`'s budget the thing that
  gates this plan — and why the `dj.c` change was the entry fee. One blob at a
  time is the version that fits; all of them at once is not.

**But the park does not fit, and that is the first thing the budget says.**
Everything parked has to lie between `_ebss` and BUFWIN, because win.o is pinned
at 0xe000:

    0xe000 - 0xc8d9   =  5927 bytes of park, all told
    less highmem.o      2202
    ------------------------------
    for the blobs       3725

and the blobs, each measured as its object less its resident set:

| driver | text | data | bss | total | resident | blob |
|---|---|---|---|---|---|---|
| `dj.o`  | 3264 | 396 | 64 | 3724 | 391 | 3333 |
| `mw.o`  | 2552 | 165 | 28 | 2745 | 165 | 2580 |
| `ide.o` | 2659 |  48 | 60 | 2767 |  48 | 2719 |
|         |      |     |    |      |     | **8632** |

8632 bytes wanted, 3725 available: **over by 4907**, and `dj.c`'s 639 is 13% of
what is missing. Nor do any two fit together (dj + ide is 6052). Exactly one
driver can be parked today, whichever one that is.

For scale, `scsi.o` (3034/146/61), which is not in the build, would want about
3095 more. `dlabel.o` (599/0/74) is shared by `mw.c`, `ide.c` and `scsi.c`, so it
stays resident whichever way this goes. `multio.o` (989/152/0) is a character
driver on its own `ciosw[]` entry, so it is a candidate but not a block one.

So either the parked set is one driver, or the blobs do not travel in the image
at all and are loaded from the root disk at boot — which needs one driver
resident to read them, and is the arrangement this plan was trying to avoid. The
budget forces that decision; **the section after this one makes it, and the
resident driver turns out to cost less than the park does.**

**Relocations are the hard part, not the addressing.** A blob linked at `OVLBASE`
is self-contained for its own internals, but it still calls resident kernel
entries — `bhold`, `iodone`, `sleep`, `di`, `copy`. Those are relocations
against symbols whose addresses are not known until the kernel is linked, and the
kernel cannot be linked until the blob exists to be embedded in it. Three ways
out, none of them equivalent:

1. **A resident vector table.** The blob calls kernel entries indirectly through
   a table of pointers that stays resident. Then the blob has no external
   references at all and links standalone with `-Ttext=OVLBASE`. Cleanest to
   build and to reason about; costs an indirect call per kernel entry.
2. **Two-pass link.** Link the kernel, harvest symbol addresses, link each blob
   against them. `mxld` has `-Ttext` and a symbol table but no input that means
   "resolve against these absolute addresses", so this means teaching it one.
3. **Build-time patching.** Link blobs with the references stubbed and fix them
   up with a tool afterwards. Effective, but the fixer becomes a thing that must
   be right, and this tree has already been bitten once by trusting a linker pass
   it had not verified.

Recommended: (1), for the same reason the tree prefers one decode in
`dlabel.c` over three — the thing that has to be correct is small and in one
place.

## The kernel object file

The budget above forces a decision, and this is it: **the modules do not travel in
the image. They are appended to the kernel object file and read out of it at
runtime.**

The file is a standard object file — the a.out `mxld` already writes, loaded by
the loader exactly as it is today — with the module region appended after it, each
module padded to a 4K boundary, running to EOF. Nothing in the header says so, and
nothing needs to:

	the object's extent is text + data + symbol table + 0x10
	the module region starts at the next 4K boundary after that
	module n is at modbase + n * 4096
	the count is (file size - modbase) over 4096

The symbol table is in that extent and is the term that is easy to drop: in the
kernel this was written against it is 9090 bytes, so leaving it out would put the
region at 0xf000 and it would start inside the table. `setdev` computes it, so
the number is not one anybody has to keep in step.

Position and count are both derived, so there is no constant for the kernel and
the file to keep in step — the same argument that put the inode in HL rather than
in a cell. The 4K slots also make the reader's arithmetic free: 4096 is a whole
number of 512-byte blocks and the page size, so the file offset, the block offset
and the in-page offset are the same number shifted.

The reader already exists. `readi()`/`bmap()` reads any offset of any inode — it
is how every file is read — so `iget(rootdev, kino)` plus a read at
`modbase + n*4096` needs no new machinery. It does need the root mounted, so
module loads happen after mount and in process context, which is where a driver's
first open is anyway.

### One driver stays resident, unavoidably

To read a module you need a working driver for the disk it lives on and a mounted
filesystem. So the driver for the device the kernel reads itself from can never be
overlaid, and neither can the console — the failure cannot be reported without
one. `multio` and whichever of `mw`/`dj`/`ide` the machine boots from are permanent
residents.

That is not a cost this arrangement introduces. The park pays it too: the kernel
has to read its root filesystem to run at all. What the park buys instead is the
ability to overlay the *root* driver as well — and the arithmetic above says it
buys exactly that one, because 3725 bytes fits one of these drivers and not two
(dj 3020, ide 2767, and not with `multio`). So the park is one overlaid and two
resident; reading from the file is two overlaid and one resident, and a fourth
driver costs nothing more.

### What is left to decide

How a major finds its module. Either the modules are in major order, or a small
resident `ovlmod[major]` holds the index. The table is `nbdev` bytes and survives
a driver being dropped or reordered; order alone is cheaper and breaks quietly.

## Which module the loader places

The section above leaves the kernel needing a resident driver for the device it
was booted from — and *which* one depends on the loader. A kernel that bakes one
in is a kernel per boot device: floppy, mw, ide, scsi, four artifacts out of one
source. That is the thing to avoid, and the way out is that the loader places the
module.

**Only one module varies.** The console is `multio` and every machine here has the
Mult I/O board, so the console driver stays resident in the kernel without making
it device-specific. What varies is the boot disk's controller alone — which is
precisely the driver the section above says can never be overlaid anyway. So this
is a one-module problem, not a four-kernel one.

**And the loader is already the right place for it.** `stand/boot` builds three
loaders today — `mwboot.com`, `djboot.com`, `ideboot.com` — from one `boot.c` plus
one io.c each. Device knowledge is already there and already a small, per-device
artifact. The kernel is the thing one least wants four of.

Two shapes for the loader to do it:

### The loader parks it

The loader reads module *k* out of the kernel file and stores it in the kernel's
own 64K, where init code copies it to a `segalloc()`d page and the region is
reclaimed as headers. No segment allocator in the loader, no window, no new
handoff — and it costs the park for the length of init, which is the 3725 that
fits **exactly one** module. That is the same arithmetic that ruled the park out
when it had to hold all of them, and this is the one case it fits.

The awkward part is the address. It cannot be derived from the object's extent
the way the module region is, because the extent already runs past BUFWIN into
the window and u pages. So it is either a constant the loader and the kernel
share, or a field in the object header.

### The loader maps it into a segment

The loader puts the module into a segment itself, which is literally "place it in
the address map". It has the whole 64K to itself before the kernel starts, so it
has a window to copy through; but it has no `segalloc`, and `segalloc()` sweeps
16 upward to 255, so any segment it takes has to be marked in `segmap[]` by the
kernel or it will be handed out again later.

### Either way the handoff outgrows one register

`HL` carries the inode. A segment number makes two values, and what survives a
third is a **pointer to a boot-info block** — the inode, the segments the loader
took, and a marker for "the loader did not say". That also disposes of the
sentinel problem: zero in a field of a block is unambiguous, where zero in HL was
doing double duty.

The kernel's entry header is already a fixed-agreement interface — `_trapvec`,
`_plist` and `_mlist` sit at 0x1001 to 0x1005 — so one more cell there is in
keeping with how the entry works rather than a new kind of coupling.

### The coupling to design first

The loader must not know a module *index*. Module ordering is a build-time
agreement between the loader and the kernel build, and it breaks silently — the
failure mode this plan keeps trying to avoid. Give each module a small
identification header — a magic and its major — and have the loader **scan** for
the one matching its own controller. Then the loaders are independent of where a
build puts the modules, and a module can be added or dropped without touching
them.

That header also has to sit in front of the devsw table described below, which
pushes the entry points off offset zero. Worth deciding as one thing.

### The baseline it has to beat

One source, four kernels, selected by a build flag. No loader change at all, four
artifacts, each self-consistent. Strictly worse than either shape above — but it
is a working fallback if the loader work stalls, and a useful rig for testing
everything else in this file, since it makes the boot driver's presence a build
choice rather than a loader behaviour.

### The assumption underneath it

The chain, as it stands: the hardware switch picks the device, which gives a
device-specific first level; that knows how to read the second level, which is on
disk contiguous and out of band; the second level reads the disk through its own
wired-in driver, finds the superblock, offers a list of kernels; one is loaded
from 0x1000 to 0xfe00; the inode is handed over and the processor enters task 0,
which is the first thing with MMU access. And then that kernel needs its root
device, whose driver therefore has to be there with it.

That last step rests on an assumption nobody has written down: **that the device
the loader read the kernel from is the device the kernel will mount as root.** It
is doing two separate jobs, and they come apart in the same case.

The first is choosing the module. If the two devices are the same, the loader's
own controller is the one the kernel needs, so the loader looks like the right
thing to ask. It is not — the kernel needs the driver for `rootdev`, which is a
property of the *kernel build*, not of the loader. Booting a kernel off a floppy
with a hard disk for root is an ordinary thing to do, and there the loader's
controller and the required module are simply different. The module should be
selected by the root major the kernel declares — in its header, or as a field the
loader can read — and the loader then loads whatever that names, reading it off
whatever device it happens to boot from, which it can.

The second is what `_kino` *means*. An inode number is only an inode number on a
device. If the kernel was booted from a floppy and its root is a hard disk, that
number names a different file on root — or nothing — and the kernel would open
the wrong one. Silently: an inode is a number, and there is no name to compare.
So the handoff should carry the boot device alongside the inode, and the kernel
should use `_kino` only when the boot device *is* `rootdev`.

Which leaves the general case costing one more resident module: the boot device's
driver, needed for nothing except re-reading the file the kernel came out of. It
cannot be overlaid, because reading it back needs the root mounted, and the root
is the other device.

The cheap first cut is to **enforce the assumption rather than support its
negation**: the kernel takes `_kino` only when the boot device matches `rootdev`,
and otherwise says so and carries on without it — no modules, no overlays, the
kernel it booted as. That is strictly less than the general case and strictly
more honest than reading a number that means something else.

### Or the installer stamps both

Everything above assumes the loader decides. It need not. `rootdev` is already a
value that belongs to the *installation* rather than to the build — it is
`main.c:32`, `UINT rootdev = 0x0300`, a compile-time constant that nothing stamps
at install time, so today changing the root device means editing source and
rebuilding the kernel. That is this same problem in its smallest form.

So let the thing that patches `rootdev` choose the driver as well. One stamp,
two facts, and they cannot disagree — because they are written together, in one
pass, by the tool that already knows what it is installing. That:

- takes the decision away from the loader entirely, and with it the assumption
  above: "boot device is not root device" stops being something the loader can
  get wrong and becomes something the installer can *say*, which is what it is.
- makes the kernel a single artifact. The module identification header then does
  the narrower job of saying what each module **is**; the stamp says which one the
  installation **wants**, and the loader only has to read the stamp and load that
  module off whatever device it booted from.

Two things to get right, both of which the tree has already been bitten by:

- **Patch the file that boots.** The caution in `disks/hdinstall/README` — "Nothing
  is patched. An earlier version of this directory had the swap device changed and
  that was a mistake" — is about a *stale copy*, the hand-copied kernel in
  `kernels/`, not about stamping as such: `create_vol` now copies `sys/unix`
  straight in precisely so the thing stamped is the thing booted.
- **Make the stamp checkable.** A stamp that is wrong is a kernel that boots and
  mounts something else, and the failure is silent because a device number is a
  number. The label already carries the geometry for exactly this reason, and the
  root device belongs beside it — so the kernel can compare what it was stamped
  with against what the disk says, and refuse rather than mount.

## The slot, and setdev

This resolves the two sections above and takes the loader out of the decision
entirely.

The format is a vanilla kernel with **one 4K-aligned slot in its text for the root
device's driver**, and **four whole 4K segments appended past the object's extent,
one per driver** — mw, dj, ide, scsi. The appended four are not loaded at boot,
being past the extent the loader reads; they are there to be copied from, and to
be read at runtime.

`setdev` is the installer's tool and it does two things: it copies the Nth
driver's 4K page onto the slot in the kernel text, and it patches `rootdev`. One
pass, two facts, written together, so they cannot disagree — which is the whole of
the answer to "which module".

**The modules are fully relocated at the slot address and have no unresolved
symbols**, which is what lets the copy be a copy. Nothing in `setdev` relocates,
and there is no linker in the tool.

That is reachable with the linker this tree already has, by linking the kernel
**once per driver with that driver in the slot** and cutting the 4K page out of
each. Every one of those links is an ordinary single-pass kernel link, so every
symbol resolves normally and nothing new is needed; the four extracted pages are
the four drivers as they sit in place. The vanilla artifact is the same link with
the slot blanked.

And `setdev` needs no constants. The slot is a symbol — `unix` is not stripped —
so the tool reads its address from the symbol table and derives the file offset;
the module region starts at the next 4K past the object's extent; module *n* is at
`modbase + n*4096`.

### What it costs

The slot is one 4K page of the kernel's text, resident always, of which about a
kilobyte is padding: dj is 3020 and the largest of the four, so 1076 bytes of it
go unused, and the smaller drivers waste more. Against a kernel that carries one
driver's text in any case, that is roughly a kilobyte for page alignment and a
one-page copy. It comes out of the same budget as `OVLSEG` and belongs in that
table — and the two are different pages: the slot is the driver that is never
swapped, `OVLSEG` is where the others appear.

The appended 16K costs the file and not the kernel. It is past the object's
extent, so the loader never reads it; the kernel reads it at runtime, when an
overlaid driver is first used.

### What it does not do

It overlays nothing. The root driver is resident on purpose — that is what lets
the kernel read its own file — and the other drivers are no better off than they
are today until there is machinery to swap them. So this is shippable on its own:
`setdev`, the slot, and the appended modules, with the overlays still to come.

And it retracts one thing above. The handoff does **not** need to outgrow a
register. A boot-info block was there to carry the boot device, because the loader
might have booted a kernel whose root was elsewhere; once the installer stamps
both the driver and `rootdev`, the boot device and the root device agree by
construction, the inode in HL is unambiguous, and the built `_kino` path is enough
as it stands.

## Building a driver page

A driver page is a self-contained object: linked on its own at `OVLBASE`, with its
**header** as the first bytes — a `struct ovlhdr` (`sys/ovl.h`) — and the driver
behind it. The resident side reaches the driver only through the header.

That is what makes the two halves independent. The resident code knows nothing
about the page's layout — not where `djstrat` landed, not how big the driver is —
only which major it is dispatching on.

And the major is enough, because it can index the array of pages directly. A
resident `ovlseg[]` (`sys/ovl.c`), indexed the way `biosw[]` and `ciosw[]` already
are, holds each overlaid driver's segment. A zero segment is a driver the kernel
still holds, whose header is a kernel object and whose data pointer is a kernel
address; that is what makes resident and overlaid one mechanism seen twice, and it
is why drivers move out one at a time.

`biosw[]` and `ciosw[]` no longer point at drivers at all. Every entry points at a
resident **trampoline** — `ovlopen`/`ovlclose`/`ovlstrat` for the block entries,
five more against `ciosw[]` — and the kernel links those tables whether a driver
is resident, is a module, or is not there at all. A trampoline recovers its major
from the device it was handed, maps that major's module (`ovlmap`), and calls the
entry the header registered; with none registered it answers the way `nodev` does.
So the kernel names no driver symbol, and `strat()` (`uio.c`) does not have to be
a funnel any more: the only place a driver's code address is held is the table in
`ovl.c` that the header filled.

The page's header is not a new ABI either. It is `struct ovlhdr` — the major, the
entries as `bvec`/`cvec` pointers, the interrupt line and handler, the tick, the
name `devname[]` reports, and a pointer to the driver's own data — and it is the
whole of the interface in both directions. **Nothing calls in.** A driver declares
itself here; it does not register itself, and `bvecset`, `cvecset`, `intrset`,
`nameset` and `tickset` are static in `ovl.c` for that reason.

None of `ovl.c`'s tables may live in the u page beside the switch tables they
feed. They are *written* when a driver is placed, and the u page must stay
read-only or fork's `segcopy` clones it per process — one table per process, each
holding the segment the driver was placed in. They belong to the kernel's own
data, which is where they are: `consts.c` links into `upage.o`, `ovl.c` does not.

The header is first in the page because it has to be at a known address, and the
one address that is known is `OVLBASE` itself. So a page is one object linked at
`OVLBASE`, with `djhdr` (or `mwhdr`, `idehdr`) declared first in it and the driver
behind it. Nothing is assembled separately to put it there.

`biosw[]` and `ciosw[]` are different shapes, three entries against five, so a
driver fills `bvec`, `cvec`, or both. `dj` is the both case — the floppy, with its
raw character mode — and one header carries the two tables.

### Init runs first, and decides whether anything else happens

The order in `ovlplace` (`sys/ovl.c`) is the load-bearing part of this. The
placement itself comes first, because init is what has to find the driver's own
data: `ovldata(major)` maps the major, and for a driver the kernel still holds
that is the resident header. Then **init runs, and nothing else is taken until it
returns zero.** The entries, the name, the tick, the interrupt line are all read
from the header only afterwards.

A driver whose hardware is not on this machine says so by failing. A driver that
fails is not registered at all: its major goes on answering the way `nodev` does,
and the only thing spent is the page. There is nothing partial to unwind, because
registration has not happened yet.

That is what lets one kernel carry a driver for a card that is not plugged in — a
SCSI driver that probes, finds nothing, and returns ENODEV costs a page of address
space and nothing else. It is also why init is allowed to be hardware only: it
runs before the header is worth anything, so it cannot be the thing that names the
driver's entries, and it does not have to be.

### The other direction

The header is how the kernel reaches the driver. A driver reaching the kernel is
the same problem seen backwards — it calls `bhold`, `iodone`, `sleep`, `di`,
`copy`, and those are relocations against symbols a page linked on its own cannot
see — and it is what `mxld`'s `-A<image>` answers: *resolve undefined symbols from
a linked image*. So a module is linked `-Ttext=OVLBASE -Aunix`, the kernel having
been linked first, and every kernel entry it names is filled in with the real
address out of the kernel's own symbol table.

That is the whole of the mechanism, and it is deliberately not a vector table. The
driver makes an ordinary call; there is no extra indirection, and there is no
second description of the kernel's interface to fall out of step with the first.
It does mean the kernel must be linked before any module is, which is the same
direction the append-to-the-kernel-file arrangement already runs.

### What else a page has to hold

A driver's state lives in its own page — that is the section above — but a driver
that needs a **borrowed window** at the same time has a problem, because only one
segment of the overlay can be the driver's own. `dj.c` is that driver: its 1K
staging exists because the controller has no scatter/gather, and it has to be
visible *beside* the buffer it copies to. If the driver's page is at `OVLSEG` and
its buffer is at BUFSEG, both are mapped and the copy works — the staging is a
*segment* apart from the data, not a page apart, so `swin`/`srel` is the thing
that would get in the way, not the overlay. Worth re-checking against the built
`dj.c` before it is the first driver moved out.

## The trampolines, the interrupt path, and the tick

All of it is built, in `sys/ovl.c`, and it is one page's worth of state:

| table | indexed by | holds |
|---|---|---|
| `ovlseg[]`, `ovlloc[]` | major | the segment a module was placed in, or the resident header |
| `ovlbvec[]`, `ovlcvec[]` | major | the entries the header declared |
| `ovlintc[]`, `ovlintseg[]` | line | the handler and the segment it lives in |
| `ovltickc[]` | major | the tick the header asked for |

The block and character trampolines are one piece of code per entry, not per
driver: each recovers its major from the device it was handed, maps, and calls
through `ovlbvec[maj]`/`ovlcvec[maj]`. A major with nothing behind it answers the
way `nodev` does, so a disk that is not loaded is indistinguishable from one that
was never there — except that a request against it completes with an error instead
of hanging, which is what a major that may be loaded later has to do.

`ovlmap(major)` returns 0 only if the major is past the table. A major with no
segment is a driver the kernel still holds: it returns the resident header and
writes no map register. Otherwise it maps `OVLSEG` and returns `OVLBASE`. **The
entry in the table is what says whether a driver is there, not the segment** —
that is the one thing to keep straight, and it is why the trampolines test
`ovlbvec[maj].open` rather than `ovlseg[maj]`.

Interrupts never go through a call site of the kernel's choosing, so the line
carries its own segment (`ovlintseg[]`) and `ovlntr` maps it before calling. The
tick is the same problem arriving by a different door, and is served the same way
by `ovlcall`. `ovlcall`'s save/restore is `image0[2 * OVLSEG]`, written the way
`newmap()` writes it — `image0` is the kernel's readable copy of the map, the
registers themselves being write-only — which is the same discipline the rest of
the kernel's window handling already uses.

The tick is **one resident clock for the whole system**, walked across the modules
that asked, not a timer per driver. `tlist[]` (`time.c`) holds five timeouts for
the entire kernel and a driver's watchdog is never stopped, so a driver that armed
its own held a slot for the rest of the boot — one per time the disk was opened.
`ovltick` takes one, only for as long as something wants one, and re-arms itself
at `TICKINT` = `HERTZ`, fast enough for the shortest period any driver wants;
a driver that wants a longer period counts its own turns (`DJTICKS` is 10, one
`djticker` per resident tick, before `djgoose` looks at the controller).

`OVLSEG` is `9`, the page at `0x9000`, and it is a property of the build rather
than a choice: the frame moves when the kernel's text grows past its page, so the
kernel link and every module link come out of one make variable (`CCFLAGS`) and
cannot disagree. Measured: the resident text ends at `0xa6d0` with the drivers
still linked in and `0x8388` without, so `0x9000` is the page that works once they
are modules and a page higher would cost the pool four fifths of itself. While
they are still resident nothing is mapped at `OVLSEG` at all — `ovlseg[]` is zero
for each of them — which is why the two states can coexist during the migration
without the frame having to be clear yet.

## What is already in place

Built and verified:

- `sys/ovl.h`, the header — the whole interface, and `struct ovlhdr` itself.
- `sys/ovl.c`, the resident half: `ovlplace`, `ovlmap`, `ovldata`, the eight
  trampolines, `ovlntr`/`ovlint0..2`, `ovltick`/`ovlarm`, `ovlattach`,
  `ovlresident`.
- The three headers: `djhdr`, `mwhdr`, `idehdr`, at the end of `dj.c`, `mw.c`,
  `ide.c`. All three register and the kernel boots with them — `disks: djdma
  hddma ide`, an mw root, and a shell.
- `sys/djinit.c`: dj's initialization split out of `dj.c` and folded into
  `highmem.o`, so `dj.o` fits a page. It reaches the driver's command block
  through `ovldata(2)` and names nothing in the module.

Still to build:

- The module link: `mxld -Ttext=OVLBASE -Aunix`, and extracting the 4K page.
- Removing the three drivers from `KERNEL_C`, and appending the pages to the
  kernel file 4K-aligned past its extent.
- Emptying `ovlres[]` once nothing is resident, and having the loader call
  `ovlattach` instead of `ovlresident`.
- `setdev`: copy the Nth page onto the slot and patch `rootdev`.

## Order of work

1. Write the module build rule and get `dj.mod` linking against `unix` with
   `-Aunix`, with no undefined symbols.
2. Move `dj` out of `KERNEL_C` first, with `OVLSEG` zeroed out of the resident
   table for it, and prove that booting from an mw root with a dj module placed
   by init still reaches a shell. `dj` is the candidate: it is the freshest, it
   has the smallest page margin, and its transfer path wants a test run with a
   floppy attached regardless.
3. Then `mw` and `ide`, then empty `ovlres[]`.
4. Only then the setdev slot, the loader reading the appended region, and the
   swapping that is the point of all of it.

## Caveats

The mechanism is built and the resident half is booted; what has not been tried
is a driver *actually placed from a module* rather than resident. The two things
most likely to be wrong when that is tried: the `-A` link, in that the module's
relocations have to come out right against a kernel that was linked without the
drivers and so has a different text extent; and the frame, because `OVLSEG` is
derived from where the text ends and that number moves the moment the drivers
leave — the kernel link and every module link must take it from one variable.
