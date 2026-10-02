# Buffer cache hashed lookup

Status: implemented.  The code is `NHASH`, `bhash[]`, `hash()` and `unhash()`
in `sys/uio.c`, with the chain link `b_hash` in `struct buf`
(`include/sys/buf.h`).  This is a record of what was built; the two places it
departs from the original plan are called out below.

## Motivation

Before this, `bget()` (`sys/uio.c`) found a cached block by walking
`blist[0..btop]` (282 buffers at present, capped at `MAXBUFS` 256) and
comparing `b->blk == blk && b->dev == dev` on every lookup.  That was O(n) per
`bread`/`bwrite`/`bawrite`/`bdwrite`, and it was also the *only* place that
ties a block number to a buffer — which is exactly what the "false hit / same
physical data" hunt is probing.

A hash table makes the hit path O(1), and gives a single, explicit place to
audit the `blk` <-> `data`/`xmem` mapping.

## Design

### Hash table

```
struct buf *bhash[NHASH];
```

- `NHASH` a power of two.  It settled at **64**, not the 128 the plan started
  with: 64 pointers is 128 bytes and the average chain is still short even at
  282 buffers.
- Hash (XOR/shift/AND only, no division — cheap on the Z80):
  `h = (blk ^ dev) & (NHASH - 1)`.
  The low bits of `blk` are already well spread because the free list is a
  consecutive descending run; `dev` is folded in so two block devices don't
  always collide.  The plan folded it in as `dev << 8` — the built hash uses
  `dev` directly.  The size and the shift are both tunable if a particular
  dev/block mix collides.

### Chain link

- Add one field to `struct buf` (`include/sys/buf.h`):
  `struct buf *b_hash;`
- Singly-linked, head-inserted per bucket.
- Do **not** reuse `b->forw`: it is live during `bsort()` and the I/O queue
  (`mw.c`), while `b_hash` must stay valid the whole time the buffer is cached.
  Removal needs a short walk, but chains average length ~1.

### Lookup (`bget`)

1. `h = hash(blk, dev)`; walk `bhash[h]` for `b->blk == blk && b->dev == dev`.
2. **Hit** -> `block(b)`, return unchanged.  (Read-vs-cache is still decided by
   `BDONE` downstream in `bread`.)
3. **Miss** -> keep the existing linear scan *only* to pick the LRU (`f` by
   `b->time`), reassign, then unlink from the old bucket and link into
   `bhash[h]`.

### Maintenance

- `binit()`: zero `bhash[]` and every `b->b_hash`.
- `bget` reassign path: unlink from `hash(old_blk, old_dev)` **before**
  overwriting `blk`/`dev`, then link into the new bucket.
- `bflush(dev)` (`uio.c`): unlink each buffer it flushes.

## Memory cost

`64 * 2 = 128 B` for the table, which lives in resident `.bss` with `bhash[]`
itself.  The chain link is one extra pointer inside each `struct buf`, and
those headers are already resident `.bss` between `_ebss` and `BUFWIN`
(`sys/main.c`) — so unlike the plan's estimate, nothing is taken from the
packed u-page budget.  Acceptable for O(1) hits.

## What this does NOT fix

The hash is keyed on `b->blk`, so a *stale* `b->blk` still produces a false
hit — the hash cannot tell that the buffer's bytes no longer belong to `blk`.
Correctness of the `blk <-> data/xmem` mapping is orthogonal; the hash only
makes lookup fast and gives a natural spot for an assertion like
"this slot still owns this block" (which is where the `dumpbuf` probe lands).
