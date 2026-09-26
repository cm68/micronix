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
difference free, and that difference is where the aligned page comes from.

`sys/dj.c` was the first instalment. Its 1K sector buffer `kbuf[1024]` was in the
kernel's own segment; it is now a `segalloc()`d page reached through the scratch
window (`SCRSEG`/`swin`/`srel`), which took `dj.o` from `text 3001 data 1419 bss
57` to `text 3264 data 396 bss 64` and moved `_ebss` from 0xcb58 to **0xc8d9** —
about 750 bytes of resident address space for one byte of state and one page of
the 1M. `sys/TODO` has the arithmetic for both budgets; the pool's cap
(`MAXBUFS`) binds before the ceiling does, so address space returned is worth
more than image bytes returned.

Measured with `mxnm unix | grep _ebss`, which is the way to re-check it — the
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
| `specs[]`, `delay[]` | **read-only tables the handler reads**: `getstat` reaches `d->specs` and `d->step`/`d->settle` |

The last row is the one that is easy to miss: 318 bytes of tables that look like
data and belong to the driver, but are read on the interrupt path. So "fits in a
page" has to be measured as text + data + bss **minus the resident set**, not as
the whole object. `dj.o` is 3724 bytes today and would fit; that does not settle
it until the split is drawn.

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
mappable, and per `KMEM.md` there is no free one: everything from `_ebss` to
BUFWIN is minted pool, and 0xd000 within that range is the scratch window mem.s
borrows and restores. So the page comes out of the pool's range — 4K at 21 bytes
a header is around 195 buffers, roughly three times what the `dj.c` change
returned. That is a decision to take deliberately, with the pool's ceiling
dropped to match, not a side effect.

## What is already in place

- `highmem.o`'s reclaim: the park-after-bss, copy-out, mint-over-it pattern.
- The u page's per-process remap, which is the shape the overlay's restore wants.
- `segalloc()`/`segfree()` for the physical pages.
- `SCRSEG`/`swin`/`srel`, the transient-mapping idiom the staging buffer uses —
  the same borrow-and-restore mem.s has always done at 0xd000.
- `intrpt.s`'s per-line stubs, which are already one indirection away from being
  mapping trampolines.

## Order of work

1. Decide `OVLSEG` and take it out of the pool deliberately.
2. Put an overlay segment in the process's map state and write it beside the u
   page's, so the restore exists before anything depends on it.
3. Do one driver end to end. `dj.c` is the candidate: it is the freshest, it has
   the smallest resident set, and its transfer path is not yet exercised — so it
   wants a test run with a floppy attached regardless.
4. Only then the other two, and the vector table, and the interrupt trampolines.

## Caveats

Nothing here has been built. The budget figures are measured; the mechanism is
argued from the code that exists. The two things most likely to be wrong when it
is tried: the relocation answer, and the size of the resident set — the second
because it is easy to count the state a driver *writes* on the interrupt path and
miss the tables it only *reads*.
