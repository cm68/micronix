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
4K. The blobs are folded into one data-only object with `-Sdata`, parked after
bss with `-Shigh`, and read in as part of the image — which is exactly what
`highmem.o` already does. At init, before `expand_bufs()` mints anything, each
blob is copied into a `segalloc()`d page and its segment number recorded. After
that the region they were parked in is just memory, and the pool mints buffer
headers over it.

The ordering is the whole trick and is not new: `highmem.o` already runs *as*
the region it is about to be reclaimed from, and the driver blobs are that with a
copy inserted between reading and reclaiming.

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
budget forces that decision; the plan does not currently make it.

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
