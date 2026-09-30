# Kernel overlay strategy

Status: plan (not implemented). Serves as the doc for when we build this.

## Context

The 64K logical address space is full. The kernel text runs 0x1000 → ~0xa000,
the data/bss plus the minted buffer headers fill to 0xd000, and 0xd000/0xe000/
0xf000 are already the copyin/out page, the buffer window, and the u page. There
is no room for more resident code.

The device drivers (`dj`, `mw`, `multio`, `memdev`, `cus`, …) are reached *only*
through the `biosw[]`/`ciosw[]` switch tables, and they never call each other.
That is exactly the precondition for overlays: at most one driver is in use at a
time, so they can all share one 4K page at the same virtual address.

## The resident leaf core — the larger target

The driver overlay above is the first, simplest instance of a larger move. The
end goal is to shrink the *resident* kernel to a single page — the u page — and
overlay everything above the copy path, so that the bulk of the 64K window
(`0x1000`–`0xefff`, 56K) is free for transient mapping: overlay slots *and*
page-cache windows.

### The self-containment invariant

Put *all* of the block-copy, page-movement, and map-setting code in the u page,
alongside the stack and the arguments it runs on. Then the copy path holds no
reference into `0x1000`–`0xefff` — not its code, not its working set — and the
region is safe to remap mid-copy, because the code doing the remap is never in
the region being remapped.

Today this fails: the leaf code sits in segments 1–2, so `segcopy`/`_zerouser`
remap exactly `0x1000`/`0x2000` and swap out the running copy routine (KMEM.md's
"debug-hostile" window). Consolidating the leaf code at `0xf000` removes every
such reference and turns the whole band into free transient space.

### The enabler: stack at the top of the u page

The u page is the last 4K (`0xf000`–`0xffff`) — where the loader parks its stack
— so filling the page with code overwrites that stack and crashes the loader.
Fix by reorganizing the page so the system stack (`char s[512]`, today the first
field of `struct user`) sits at the very top (`0xfe00`–`0xffff`) and the code
fills `0xf000`–`0xfe00`. The loaded image then stops at `0xfe00`, below the
loader's `sp`, and the stack region is runtime space reused by the kernel after
boot. This also hands the resident core ~3.5K of code — its entire residency
budget.

### Interrupt residency

Interrupt-level code cannot be overlaid: an interrupt can land while
`0x1000`–`0xefff` points at a cache window. The handler *and the data it
touches* must be resident in the u page. `mem.s` already `di`s across the
remap+copy, so the copy is atomic with respect to interrupts; the constraint is
where the handler and its state live, not just the handler.

The resident core is therefore: leaf primitives + interrupt handler + its data +
`u` + stack. Everything else — syscall dispatch, the filesystem, the buffer
cache, the drivers — is overlay-able.

### Loading is init-time, not runtime

The overlay segments are loaded once, at boot, by init-time code: it `segalloc`s
one segment per module, reads each module's code+data in, and returns. That
init-time code is then itself overlaid — its address space becomes bss kernel
data (the buffer headers, tables) — exactly the init-only/highmem pattern
`main_init.c` already uses. So loading is a one-time boot operation in
reclaimable init code; runtime swap-in/out stays a pure MMU remap, never a disk
load.

### The overlay/cache partition

`0x1000`–`0xefff` then serves two masters: overlay slots for running kernel code,
and windows for the page cache. The resident trampoline remaps-and-jumps into an
overlay; when no overlay is active the same pages window cache segments. The
partition — how many pages pinned as overlay slots vs. cache window — is open,
and is governed by the largest overlaid module (the "region" idea below, now
sized to the fs rather than just SCSI).

### The blocking problem, generalized

The driver seam kept overlaid code non-blocking because return-into-a-swapped
overlay is fatal. The whole kernel has no such luxury — syscalls and the
filesystem sleep constantly — so overlaying them re-opens the return problem the
seam closed. This is the main open question for kernel overlays; the non-blocking
corollary does not carry over. Candidates to revisit: pin the overlay for the
syscall's whole lifetime, save/restore the return path in the resident
scheduler, or overlay only the leaf-most non-blocking code and keep the blocking
middle resident.

### Payoff

A 56K transient region is what a capable page cache wants: it dissolves the
header/cache scarcity ZEROCOPY.md identifies as the binding constraint, and it is
the other half of zero-copy shared text (text pages map into process space; the
cache windows reach the pool segments they live in).

## Core idea

Reserve one 4K logical page — the *overlay page*. Every driver is linked to that
same page, code and private statics together. A resident trampoline remaps the
page to the target driver's physical segment (one MMU register write) and jumps
in. "Swap in/out" is therefore a **remap, not a disk load**: each driver's
code+data sits permanently in a physical 4K segment (the MMU has 1 MB of physical
space behind the 64K window), and the overlay page just points at whichever one
is active.

## The top-end / bottom-end seam (the primary split)

A driver is two halves that meet only through shared state + `sleep`/`wakeup`,
never by calling each other:

- **Top end** — the synchronous entry (`open`/`close`/`read`/`write`/`strat`),
  reached through the switch table, in process context.
- **Bottom end** — the interrupt FSM, in interrupt context, advancing the device
  through its states (SCSI phases, the MW/DMA sequence) on each interrupt.

Because the halves never call each other they are independent link units, and the
overlay boundary falls exactly on that seam:

| Unit | Where | Why |
|------|-------|-----|
| Bottom end (FSM) | resident | async — runs when the overlay isn't mapped |
| Shared state (CDB, result, FSM state, queues) | resident | the interface both halves touch |
| Top end (`strat`/`open`/`close`) | overlaid | synchronous, non-blocking — the wait is in resident `bwait` |

This one split dissolves both classic overlay hazards at once — the **interrupt
problem** (the FSM is resident, so always reachable) and the **return problem**
(the overlaid top end is non-blocking, so its lifetime is one atomic span). The
shared state between the halves is a small resident ABI: laid out once, linked
against by both.

## Components

### Overlay page
- A fixed 4K logical page reserved for the overlay (page 4, 0x4000–0x4fff, say;
  the exact page is a link-time choice). All overlaid drivers are linked to this
  base, so they overlap each other in logical space.

### Physical backing
- `segalloc()` hands out one 4K segment per driver, permanently resident in the
  1 MB physical space. Remapping never moves or re-loads the bytes.

### Trampolines (resident)
- One small thunk per public driver function (`mwstrat`, `muwrite`, …).
- The thunk calls `swap_in(driver)`, then jumps to the real function at
  `OVERLAY + offset`. The switch tables point at the thunks, so nothing above the
  driver layer knows the code moved.

### Overlay manager (resident)
```
swap_in(seg) { map0[2*OVL_PAGE] = image0[2*OVL_PAGE] = seg; }
```

### Static data
- Lives on the overlay page and is remapped with the driver.
- It is preserved across swaps because its physical segment never changes —
  remapping changes only the logical view, not the contents.
- *Caveat:* only the top end's *private* statics go on the overlay. The *shared
  state* the bottom-end FSM touches (the active command, the result/sense buffer)
  must be resident so the resident FSM can reach it — see the seam above.

## The non-blocking corollary (call is easy, return is hard)

The seam places the top end in the overlay only because it is non-blocking; here
is why that matters. `swap_in` + jump is two instructions — the easy call. The
hard part is the overlaid function's *lifetime* — everything up to and including
its `ret` — and two things break it:

1. **Return-into-a-swapped-out-overlay.** A thin wrapper like `muwrite` (overlay)
   calls resident `ttywrite`, which sleeps. While it sleeps, another process's
   trampoline maps a different driver over the page. When `ttywrite` wakes and
   `ret`s to `muwrite`'s address, the bytes there are the wrong driver.
2. **Sleep-in-overlay** is the same failure from the inside.

So an overlaid function must be **non-blocking** — it runs to completion as one
atomic span with its overlay mapped. That is the corollary of the seam: the
overlay holds the non-blocking top end (block `strat`/`open`/`close`, the
per-char `put`/`start`/`stop` hooks), while the blocking top end
(`ttyread`/`ttywrite`, `bwait`, the `muread`/`muwrite` wrappers) stays resident.

## Packing (knapsack)

Fitting modules of differing sizes into 4K chunks is bin-packing. It's NP-hard in
general but trivial for ~6–8 drivers (first-fit-decreasing or exhaustive). The
binding constraint is not the byte sizes but the blocking boundary above: a module
that must stay resident can't be combined with overlaid modules, so the partition
is cut along that line first.

## Big drivers (SCSI): overlay region, not one page

A driver larger than 4K — the SCSI driver is the motivating case — spans several
4K chunks, and because its functions call each other those chunks form a *group*:
swapped in and out together. So the overlay is not a single page but a **region**
sized to the largest group, and each driver is a group of 1..N pages packed into
it. The knapsack is over whole groups, not independent items.

The SCSI driver splits along the seam:

- **Overlaid (top end):** `scsistrat` — build the CDB, map the data buffer, kick
  the controller. Non-blocking.
- **Resident (bottom end, the FSM):** `scsiint` — the phase engine (selection,
  command, data, status, message) + error/sense recovery + the active-command
  state (in-flight CDB, DMA descriptor, result/sense buffer).

The phase engine is typically the bulk of a SCSI driver, so it is the resident
part; the overlay reclaims the top ends of *all* drivers into one shared region.
If the error/sense handling is large, either keep it in the FSM (resident), or
defer it to the `strat` context (overlaid) to keep `scsiint` minimal — the latter
means the error path runs only while the overlay is mapped.

## The interrupt path is resolved by the seam

The disk drivers (`dj`, `mw`) and `multio` have interrupt entry points (`djint`,
`mwint`, `mumint`) that run asynchronously — a disk DMA completes while the
console driver is the active overlay. The seam handles this by making the bottom
end (the FSM) resident, so the interrupt entry is always reachable. The one thing
to get right is that the *shared state* the FSM touches (`mwbuf`, `cmd`, `mws[]`,
the active CDB) is resident too — otherwise a resident handler would chase
pointers into a swapped-out overlay.

Fallbacks if a driver's bottom end can't be made resident:

1. **Interrupt dispatcher swaps the overlay first.** The dispatcher already knows
   which device interrupted, so it writes the right segment before calling
   `mwint` — one register write, but the dispatcher must stay re-entrant-safe.
2. **Overlay only the non-disk drivers** (`multio`, `memdev`, `cus`, …) and leave
   `dj`/`mw` resident. Simplest; recovers less space.

## Build / linker

- **Preferred:** extend the linker to understand overlay sections. It links each
  driver at the overlay base, emits a per-driver segment table and the
  public-name → overlay-offset map, and generates the resident thunks. This is
  the v6 overlay-linker model and removes the need for hand-written thunks.
- **Fallback (no linker change):** compile each driver under a name prefix
  (`mwstrat` → `_ovl_mwstrat`), hand-write the thunks, and fill the switch tables
  with the thunks.

## What gets overlaid

The non-blocking top ends of the block/char drivers reachable only via the
switches (`dj`, `mw`, `multio`, `memdev`, `cus`, …) and any future driver. Not
overlaid, in this first phase: the core kernel, the buffer cache, the
trampolines, the switch tables, the bottom-end FSMs, and the shared state those
FSMs touch.  The full target overlays everything but the resident leaf core —
see "The resident leaf core" above.

## Trade-offs

- **Gain:** one 4K region holds N drivers' top ends; the resident image shrinks by
  (sum of top-end sizes) − region size.
- **Cost:** one MMU register write per driver entry (negligible), plus a few
  bytes per resident trampoline.
- **Risk:** the seam is the only correctness hazard — the shared state must be
  resident or the resident FSM chases pointers into a swapped-out overlay.

## Verification

1. Boot with the disk drivers overlaid; run a full build (heavy block I/O) to
   exercise `dj`/`mw` under load.
2. Exercise every driver (console, `/dev/mem`, etc.) to confirm the switch tables
   dispatch through the thunks.
3. Confirm statics survive across swaps (e.g. the MW geometry / `curtrk` state
   persists between two `mwstrat` calls that are separated by console I/O).
