# Kernel memory architecture

How the Micronix kernel lays out memory, both today and where it is headed.

## Address space

The Z80 has a 64K logical address space (16 x 4K segments).  Physical
memory is 1M (256 x 4K segments), reached through the Morrow memory
map:

    MAP0/IMAGE0  0x0600 / 0x0200   task-0 (kernel/supervisor) map
    MAP1/IMAGE1  0x0620 / 0x0220   task-1 (user) map

`newmap()` (malloc.c) writes a process's `mem[16]` into MAP1/IMAGE1 and
sets `map0[2*USERSEG]` to its u segment.  The kernel runs under MAP0;
segments 1-2 (0x1000, 0x2000) are scratch "windows" that mem.s remaps
to reach user space (or another segment) during copies.

Segments 0-15 are the kernel's own 64K; segments 16-255 are the pool
that `segalloc()`/`segfree()` (malloc.c) hand out for user pages, the
buffer cache, and swap.

## Current layout (post u-move, leaf code started)

```
0x0000 - 0x0fff   ROM + I/O page            (not in the image)
0x1000 - 0x8b82   text                      35714 bytes
0x9b82 - 0xcdf2   data + bss
0xcdf2 - 0xefff   buffer pool               binit() places 512-byte blocks
0xf000 - 0xf274   struct user u             629 bytes
0xf275 - 0xf8ef   leaf code                 mem.s + inout.s + libccc runtime
0xf8f0 - 0xffff   free                      future leaf code; loader-stack slack
```

`_usrtop`/`_memtop` = 0xf000 (uhdr.s); the kernel image must not reach
0xffff because the HD-DMA loader parks its stack there.

## Target layout

```
seg 12 (0xc000)   end of kernel text+data+bss
seg 13 (0xd000)   scratch / user window
seg 14 (0xe000)   buffer-cache window
seg 15 (0xf000)   u + leaf code + constants
```

Two windows total: the buffer-cache window (0xe000) and the scratch
window (0xd000).  Copies are window<->window — copyout maps the buffer
page into 0xe000 and the user page into 0xd000 and ldir's between them,
no staging.  The buffer side of a copy never spans a page (512-byte
blocks, page-aligned); the user side can span (up to 2 pages), done 2
at a time.  The scratch window is at 0xd000, not 0x1000, because
segments 1-2 hold the kernel text and remapping there during copyout
swaps out the running code (debug-hostile).

## The u page (segment 15)

`struct user u` (629 bytes) plus pure-text leaf code and read-only
constants, all in one 4K segment remapped per process via
`map0[2*USERSEG]`.  Nothing in the page may be writable data, or fork's
`segcopy` clones it per process.

Filling it, from the leaf code inward:

- libccc runtime  ~1004 bytes (csv/cret/indir, q*, ldiv/lmod/amul/adiv,
  tramp, swdisp/swdispw/swtab/swtabw/swidx, oarg) — pure text.
- mem.s          getbyte/putbyte/copyin/copyout/memrw/segcopy/zerouser
- inout.s        in()/out()
- sub8.s         saveframe/setframe/zero/copy/di/ei — needs a code/data
                 split (dicount + "di < 0" string stay in kernel data)
- intrpt.s       vector table + dispatch — needs 32-byte alignment and
                 an i8259 retarget
- static strings, syssw, biosw/ciosw, specs — read-only data (audit for
  writes first)

**Leaf code must be assembled as `.data`, not `.text`**: the loader
reads text and then data, so text parked in the page would be
overwritten by the data pass.  This is the one non-obvious mechanism.

**Not movable**: mio.s — self-modifying (patches OUT/IN port operands)
plus mutable state.

## Buffer cache

Currently a contiguous pool above `_ebss` (binit() computes
`nbuf = (usrtop - ebss) / 512`), ~15 blocks.

Target: a 64K cache (16 pool segments, 128 x 512-byte blocks — 48K is
fine too) reached through the single 0xe000 window.  Block `i` has
`data = 0xe000 + (i&7)*512` and `xmem` = its page's physical segment.
The buffer headers (`blist`) stay static in main.o.  DMA ignores the
window: physical = `xmem<<12 | (data & 0xfff)`.

A fork can then reuse a filled-out page: keep a freelist of u pages
that already carry the leaf code, copy only the 629-byte u struct, and
skip the text copy (see TODO).

## Constraints

- **Loader stack**: mwboot1.s parks sp at 0xffff and loads the kernel in
  512-byte blocks from 0x0ff0.  The image must not reach the top, or the
  last block wraps its DMA write into the rom and walks over the stack.
  This is a boot-time wrinkle, not an architectural brake — the loader's
  stack can move (backward-compatible: it reads the same header and
  loads contiguously), and after the leaf-code move there is ~2.8K of
  slack anyway.
- **48K fit**: for the scratch window at 0xd000, text+data+bss must fit
  0x1000-0xcfff.  Gated by the leaf-code move above shrinking the text.
- **.data, not .text**: leaf code in the u page (see above).
