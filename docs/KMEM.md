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

## Current layout

The target below is what the kernel links to now: the u page moved up
to segment 15, so the two adjacent pages are the buffer window (14) and
the scratch window (13) under it.

```
0x0000 - 0x0fff   ROM + I/O page            (not in the image)
0x1000 - 0x9e73   text                      36467 bytes
0x9e73 - 0xc8d9   data + bss                10854 bytes; ends at _ebss
0xc8d9 - 0xdd31   buffer pool               248 headers minted off _ebss
0xdd31 - 0xdfff   free                      719 bytes
0xe000 - 0xefff   buffer-cache window       BUFSEG; bwin() maps here
0xf000 - 0xfcde   leaf code + constants     USERSEG; 3294 bytes
0xfcde - 0xfede   ustack                    512 bytes, .bss
0xfede - 0xff53   struct user u             117 bytes, .bss
0xff53 - 0xffff   free                      172 bytes; loader-stack slack
```

`blist` is the last object in .bss: the 8 boot headers at 0xc831..0xc8d9,
which is why the pool's minted range starts at `_ebss`.  With them the
pool is 256 headers, or 5376 bytes from 0xc831 to `btop` = 0xdd31.

The pool is 719 bytes short of BUFWIN and that is not a coincidence: at
256 headers `MAXBUFS` (main.c) binds before the ceiling does, so the
pool is sized by the count and not by memory.  Minting to the ceiling
alone would be 282 headers, so the count binds 34 short of it - it was 3
short before the 1K staging buffer left the kernel, which is the change
that made the ceiling stop mattering.  See OVERLAY-DRIVERS.md.  The
0xe000 page is the wall for the resident kernel as well - `-Shigh` parks
highmem.o (2202) at `_ebss` and win.o is pinned there, so the kernel
links only while `_ebss <= 0xe000-2202` = 0xd766.  Today's slack is
0xd766-0xc8d9 = 3725 bytes, none of it reachable by the cache, and it is
where the driver blobs would have to park.  See TODO, the scsi section.

The file's data stream runs from 0x9e73 to 0xfcde - through the pool and
the window page, which carry no image bytes - and the boot loader reads
it as one 119-block flat image.  `_usrtop`/`_memtop` = 0xffff (uhdr.s).

## Window layout

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

0xd000 is *borrowed*, not reserved, which is why the map above shows the
pool running through it.  It was the superblock's window once; the
superblock is plain memory now (getsb, uio.c), so the page's only
remaining claim is the mapping a copy takes and mem.s puts back.  main.c
has the same note where the pool's ceiling is set.

## The u page (segment 15)

`struct user u` (117 bytes) and its 512-byte system stack, ustack, plus
pure-text leaf code and read-only constants, all in one 4K segment
remapped per process via `map0[2*USERSEG]`.  Nothing in the page may be
writable data, or fork's `segcopy` clones it per process.

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

The pool is not itself resident: the 8 boot headers are `blist` (the
last .bss object, textpad.s) and `expand_bufs()` (main.c) mints the rest
contiguously off `&blist[8] == _ebss`, up to `BUFWIN` — the base of the
window's own page, because `bwin()` holds a segment there for the whole
of a buffer access and a header in that page would be read back as
buffer data.  The count is what binds, not that ceiling: `MAXBUFS`
(256, main.c) is reached first, so the pool is 8 boot headers plus 248
minted, running 0xc831 to 0xdd31, the first 2202 bytes of which are over
the reclaimed init-only object, and 719 bytes of the `_ebss`..BUFWIN
region go unused.  Minting to the ceiling alone would be 282, so the
count binds 34 short of it - it bound by 3 before the 1K staging buffer
left the kernel, which is what moved the ceiling out of the way and left
the pool sized by `MAXBUFS` alone.  The 719 bytes are growth room: the
next kilobyte of kernel growth spends them and then starts eating
buffers, at 21 bytes each.  See TODO, the scsi section.

The blocks are 512 bytes each, minted `data = BUFWIN + (i&7)*512` with
`xmem` = a fresh page from `segalloc()` (8 blocks per page).  DMA
ignores the window: physical = `xmem<<12 | (data & 0xfff)`.

Raising the ceiling to `_upage` (0xf000) doubles the pool and needs the
window borrowed at every site that uses it — see TODO.

A fork can then reuse a filled-out page: keep a freelist of u pages
that already carry the leaf code, copy only the 629 bytes of ustack and
u, and skip the text copy (see TODO).

## Constraints

- **Loader stack**: mwboot1.s parks sp at 0xffff and loads the kernel in
  512-byte blocks from 0x0ff0.  The image must not reach the top, or the
  last block wraps its DMA write into the rom and walks over the stack.
  This is a boot-time wrinkle, not an architectural brake — the loader's
  stack can move (backward-compatible: it reads the same header and
  loads contiguously), and the image the kernel links to now stops at
  0xfcde, 768 bytes clear of the loader's deepest sp (0xffde).  What
  binds the leaf code today is the u page's own 4K, not the loader.
- **48K fit**: for the scratch window at 0xd000, text+data+bss must fit
  0x1000-0xcfff.  Gated by the leaf-code move above shrinking the text.
- **.data, not .text**: leaf code in the u page (see above).
