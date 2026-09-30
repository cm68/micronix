# MAPBUG — one physical page handed to two logical pages

## Symptom

The native self-hosted `make` dies with malloc's "out of memory" while the
heap has plenty of room.  The failure is a corrupted free-list: walking it
from block `0x8053` reads a bogus next-pointer and the allocator bails.

## Evidence

The page table is the `maps[]` array — the Morrow MPZ80's map registers
(hwsim `getpte()` keys it by `pte = (task<<5) + (page<<1)`).  The trace shows
the kernel writing the *same physical page into two different logical pages*
of task 1:

```
5928940695: map register 630 write 6b task 1 page 8   ← 0x80 → physical 0x6b
5928941339: map register 632 write 6b task 1 page 9   ← 0x90 → physical 0x6b  (same)
5929097459: map register 630 write 6c task 1 page 8   ← 0x80 → 0x6c
5929098103: map register 632 write 6c task 1 page 9   ← 0x90 → 0x6c  (same)
   ...
6067950949: map register 630 write 70 task 1 page 8   ← 0x80 → 0x70
6067951593: map register 632 write 70 task 1 page 9   ← 0x90 → 0x70  (same)
```

`map register 630` is `maps[0x30]` = task 1, page 8; `632` is `maps[0x32]` =
task 1, page 9.  Both entries hold the same segment, and that value marches
up (`0x6b … 0x70`) every time `grow()` runs.  `0x70 << 12` = `0x070000`, the
physical page named "page 7" — so both user pages resolve to the same
physical page.  Writes to `0x8053` and `0x9053` land on the same byte,
which is how the sbrk path's `alloct->ptr = allocs` (at `0x9053`) stomps the
block header at `0x8053`.

## Cause

`make`'s heap sits in pages 8 and 9 (logical `0x8000`, `0x9000`).  Both pages
carry `per == GROW`, and every `GROW` page maps to a single shared "grow
segment" — that sharing is *by design*, not the bug:

```c
/* sys/malloc.c, grow() */
grow = segalloc();                      /* one segment ... */
m = u.p->mem;
for (i = 0; i < 16; i++)
    if (m[i].per == GROW)
        m[i].seg = grow;                /* ... for every GROW page */
```

The mechanism that is supposed to separate them is the MMU's **grow
permission bit** (RI10).  A page whose permission byte is `07` — full access
plus RI10 — completes a write *and* traps to task 0 (the MTRAP fault).  That
trap is the whole point: `fault()` → `grow()` privatizes the touched page
(keeps the shared segment for it) and hands the still-empty pages a fresh
grow segment.  It is a copy-on-write page.

hwsim had not implemented the trap half.  `get_byte()`/`put_byte()` called
`getpte()` for the address translation and then `physread`/`physwrite`,
**without looking at the permission byte `getpte()` returned**.  So a user-mode
store into a GROW page completed with no trap, `fault()` was never called for
the heap, and pages 8 and 9 stayed welded to the one shared segment for the
life of the process — the aliasing the trace shows.

This is exactly why usersim does not reproduce it: usersim has a trivial 64K
flat map with no MMU, so there is no grow segment and nothing to alias.

## The fix — implemented

The permission byte is now enforced in the memory accessor
(`src/hwsim/d1/mpz80.c`): the read path blocks or traps at `:1133-1149` and
the write path at `:1355-1366`, latching `mem_pending_fault`, and
`take_pending_trap()` (`:795-810`) raises the deferred MTRAP at the
instruction boundary.  This is the design.

Each page entry is two bytes: the even byte is the physical page, the odd
byte is the permission field.  The trap-address register (0x400) is a
single-stage shift register — upper nibble = current page, lower nibble =
previous page — shifted on every mapped bus cycle, so on a fault it already
holds the `(faulting_page << 4) | previous_page` pair that `fault()` reads.

The permission matrix enforced:

| perm (`attr`) | read | write | execute |
|---------------|------|-------|---------|
| NONE (0)      | block | block | block |
| r/o (1)       | ok   | block | block |
| execute (2)   | block | block | ok   |
| FULL (3)      | ok   | ok   | ok   |
| GROW (7)      | ok + **trap** | ok + **trap** | ok + **trap** |

- **GROW** (RI10, `attr & 0x4`): do the access, then raise the fault.
- **blocked access**: don't do the access, raise the fault.
- The fault is a **deferred MTRAP** — `trap((ST_RESET & ~ST_INT) | ST_R10)` —
  taken at the instruction boundary.  For GROW the access already completed
  (the write landed), so no restart is needed; `fault()` → `grow()` just
  repoints the map.  Clearing `ST_INT` is what makes `trap()` treat it as a
  deferred trap (resume at the next instruction) rather than the halt trap's
  immediate `pc+1` convention.

This is exercised end-to-end by the TRAPTEST disk (`WR10`, `RR10`, `XR10`,
`NOACCESS`, `RDONLY`, `XONLY`).

## A note on zeroing

`grow()` hands the still-empty pages a fresh but *unzeroed* segment —
`segalloc()` returns whatever its previous owner left.  That is correct: C
guarantees zero only for `.bss` (which `zerouser()` clears at exec), never
for the heap, so an indeterminate grow page is harmless.  The mechanism only
has to repoint the still-empty pages off the now-dirty shared segment, which
`grow()` already does — no zeroing is required.
