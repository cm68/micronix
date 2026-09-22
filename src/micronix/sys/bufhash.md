# Buffer cache hashed lookup — design

Status: plan (not implemented). Serves as the doc for when we cut this over.

## Motivation

`bget()` (`sys/uio.c`) finds a cached block by walking `blist[0..btop]` (112
buffers) and comparing `b->blk == blk && b->dev == dev` on every lookup.  That
is O(n) per `bread`/`bwrite`/`bawrite`/`bdwrite`, and it is also the *only*
place that ties a block number to a buffer — which is exactly what the "false
hit / same physical data" hunt is probing.

A hash table makes the hit path O(1), and gives a single, explicit place to
audit the `blk` <-> `data`/`xmem` mapping.

## Design

### Hash table

```
struct buf *bhash[NHASH];
```

- `NHASH` a power of two.  Start at 128: with 112 buffers the average chain
  is under 1, and it only costs 256 bytes.
- Hash (XOR/shift/AND only, no division — cheap on the Z80):
  `h = (blk ^ (dev << 8)) & (NHASH - 1)`.
  `dev << 8` is a byte move; the low bits of `blk` are already well spread
  because the free list is a consecutive descending run.  The `<< 8` (and the
  `NHASH` size) are tunable if a particular dev/block mix collides.

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

`128 * 2 = 256 B` (table) + `112 * 2 = 224 B` (links) ~= **480 bytes**, taken
from the packed u-page budget.  Acceptable for O(1) hits.

## What this does NOT fix

The hash is keyed on `b->blk`, so a *stale* `b->blk` still produces a false
hit — the hash cannot tell that the buffer's bytes no longer belong to `blk`.
Correctness of the `blk <-> data/xmem` mapping is orthogonal; the hash only
makes lookup fast and gives a natural spot for an assertion like
"this slot still owns this block" (which is where the `dumpbuf` probe lands).
