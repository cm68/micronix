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

Whatever the interrupt path touches must be resident, or a resident handler
chases pointers into a page that now belongs to another driver. That is not only
the transfer state. In `dj.c`:

| resident | why |
|---|---|
| `dm[NDRIVES]`, `curbuf`, `qhead`, `qtail` | the queue and drive state the handler walks |
| `kdev`, `ksec`, `kw`, `kseg` | the staging state the handler updates |
| `djcomm[26]` | the controller command block, shared with `memio` |
| `specs[]`, `delay[]` | **read-only tables the handler reads**: `getstat` reaches `d->specs` and `d->step`/`d->settle`; 297 + 32 = 329 bytes |

The last row is the one that is easy to miss: 329 bytes of tables that look like
data and belong to the driver, but are read on the interrupt path. (An earlier
draft of this file said 318; `specs[]` is 27 rows of 11 bytes.) So "fits in a
page" has to be measured as text + data + bss **minus the resident set**, not as
the whole object. Summed, this table is **391 bytes**, so `dj.o`'s blob is 3333.
It fits a page comfortably — the page turns out not to be what binds.

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

	the object's extent is in its own header (text + data + 0x10)
	the module region starts at the next 4K boundary after that
	module n is at modbase + n * 4096
	the count is the file size over 4096

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

## Building a driver page

A driver page is a self-contained object: linked on its own at `OVLBASE`, with a
device switch entry as its first bytes — a table of the driver's own entry
points. The resident side reaches the driver only through it.

That is what makes the two halves independent. The resident code knows nothing
about the page's layout — not where `djstrat` landed, not how big the driver is —
only which major it is dispatching on.

And the major is enough, because it can index the array of pages directly. A
resident `ovlseg[]`, indexed the way `biosw[]` and `ciosw[]` already are, holds
each overlaid driver's segment, and the dispatcher is then one piece of code for
all of them:

	seg = ovlseg[major];
	if (seg) {
		swap in seg;
		jump through *(struct biovec *) OVLBASE;
	} else {
		call biosw[major] as today;
	}

A zero entry means "resident", so the two kinds coexist and drivers move one at a
time. That is the migration path, and it is the reason this wants to be an array
of segment numbers rather than a set of per-driver thunks.

For block devices there is already a single funnel to put that in: `strat()`
(uio.c:368) is the only route to a strategy routine, so the block half is one
place. Character devices have no such funnel — `ciosw[]` has five entries and
several call sites — so the block side is the one to do first, and `dj.c` is a
block driver.

The page's header is then not a new ABI at all. It is a `struct biovec` or a
`struct ciovec` (`sys/con.h`) at `OVLBASE` — the same shapes the switch tables
hold — which is what makes "jump through table[slot]" a single dereference.

One thing has to be placed: `ovlseg[]` is *written*, at init, so it cannot live in
the u page beside `biosw[]` and `ciosw[]`. That page must stay read-only or
fork's `segcopy` clones it per process. It belongs in the kernel's own data, and
it is `nbdev + ncdev` bytes of it — or 256 for one array covering both.

The table is first in the page because it has to be at a known address, and the
obvious one is `OVLBASE` itself. So a page is two objects linked together: a small
assembly file emitting the table with `.defw` against the driver's entry points —
resolved inside the standalone link, needing nothing from the kernel — and then
the driver.

`biosw[]` and `ciosw[]` are different shapes, three entries against five, so the
table follows the class the driver is registered in. A page is one or the other;
a device that is both is two drivers, as it is today.

### The other direction, which the table does not solve

The table is how the kernel reaches the driver. It says nothing about how the
driver reaches the kernel, and that is the harder half: the driver still calls
`bhold`, `iodone`, `sleep`, `di`, `copy`, `swin`, and those are relocations
against symbols a standalone link cannot see.

The symmetric answer is a resident vector table the driver calls through, and what
makes it work is a **fixed absolute address**. If the table's address is a
constant, the driver links against the constant and the circle is broken. If it is
wherever the linker put it, the kernel must be linked first and the driver second
— the two-pass build, and a change to `mxld`.

The tree already has a home for pinned read-only kernel constants: the u page,
where `syssw`, `biosw` and `ciosw` live. It is identical across processes, which
is the property a vector table needs, and pinning it means an absolute symbol
rather than a linker-chosen offset — so a later change to the kernel's layout
cannot move it out from under the drivers.

### What else a page has to hold

A driver's *state* stays resident — that is the resident set above — but a driver
that needs a scratch page of its own has a problem once `OVLSEG` is `0xd000`,
because both kernel windows are then spoken for: 0xd000 is the driver's own code
and 0xe000 is whatever buffer it is copying to. `dj.c` is that driver. Its 1K
staging exists because the controller has no scatter/gather, and it has to be
visible *beside* the buffer it copies to, which is why it does not sit in BUFSEG
today. Overlaying `dj.c` therefore needs one of: room in its own page for the
staging, a page of its own, or a driver whose transfer needs only one mapped
window. Worth settling before `dj.c` is picked as the first one.

## The switch tables and the trampolines

`biosw[]`/`ciosw[]` are in `consts.c`, which links into `upage.o` — the
per-process u page — so they are read-only data in a remapped page, which is
fine. Each entry points at a **resident thunk**: swap in that driver's segment,
jump to `OVERLAY + offset`. The thunk is a few bytes and lives in kernel text;
nothing above the driver layer knows the code moved.

The open question is which logical page `OVLSEG` is. It has to be 4K-aligned and
mappable, and there is no free one: the kernel holds 0x1000 to 0xc8d9, and
everything from `_ebss` to BUFWIN is pool. The only 4K-aligned address in the
free range is **0xd000**, which is the scratch window — so `OVLSEG` and `SCRSEG`
would have to be the same page.

That is less alarming than it sounds, because mem.s restores the mapping from
`IMAGE0[26]` after every borrow rather than restoring bytes; if the overlay's
segment is what `IMAGE0[26]` holds, the restore puts the driver back for free. It
does mean an overlaid driver must never call `swin`/`srel`, which *replace*
`image0[26]` instead of restoring it — which is already why `dj.c`'s staging
lives in BUFSEG and not the scratch page.

The cost lands on the pool, and `expand_bufs()` mints contiguously from
`&blist[8]`, so whichever page is taken it has to be excluded from the mint. The
three ways, by what each costs against the 256 buffers the pool holds now:

| `OVLSEG` | `_ebss` must reach | extra reclaim | minted | total pool |
|---|---|---|---|---|
| 0xd000, as it stands | 0xc8d9 (now) | none | 87 | 95 |
| 0xc000 | 0xc000 | **2265** | 195 | 203 |
| 0xd000, pool above it | 0xbba8 | 3377 | 248 | 256 |

4096 bytes at 21 bytes a header is the 195 this file used to quote, and 195 is
what a *free* page is worth — but only when it is not a hole in the mint. As a
hole at 0xd000 it is worth 161 buffers (256 down to 95); moved to 0xc000, where
the pool can sit above it, it is worth 53 (256 down to 203) for 2265 bytes of
`_ebss`. The middle row is the one to aim at: it pays for the page out of address
space instead of out of the cache, and it is 3.5 times what `dj.c` has reclaimed
so far. The third row buys the last 53 buffers back at 21 bytes each — the
standing rate, and no better.

## What is already in place

- `highmem.o`'s reclaim: the park-after-bss, copy-out, mint-over-it pattern.
- The u page's per-process remap, which is the shape the overlay's restore wants.
- `segalloc()`/`segfree()` for the physical pages.
- `SCRSEG`/`swin`/`srel`, the transient-mapping idiom the staging buffer uses —
  the same borrow-and-restore mem.s has always done at 0xd000.
- `intrpt.s`'s per-line stubs, which are already one indirection away from being
  mapping trampolines.

## Order of work

1. Decide `OVLSEG` and take it out of the pool deliberately — and decide it
   together with the parked set, because the two compete for the same bytes.
   Getting `_ebss` to 0xc000 is 2265 bytes of reclaim and is the partner of the
   0xc000 page; the park, at one blob, is affordable at 3725.
2. Put an overlay segment in the process's map state and write it beside the u
   page's, so the restore exists before anything depends on it.
3. Do one driver end to end. `dj.c` is the candidate: it is the freshest, it has
   the smallest resident set, and its transfer path is not yet exercised — so it
   wants a test run with a floppy attached regardless.
4. Only then the other two, and the vector table, and the interrupt trampolines.

## Caveats

Nothing here has been built. The budget figures are measured; the mechanism is
argued from the code that exists. The three things most likely to be wrong when
it is tried: the relocation answer; the size of the resident set, because it is
easy to count the state a driver *writes* on the interrupt path and miss the
tables it only *reads*; and the reclaim, which is measured at 639 bytes against
the 2265 the OVLSEG page wants, so the budget is the part that is *known* to be
short rather than the part that might be.
