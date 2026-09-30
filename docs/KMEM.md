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

The offsets below are a snapshot of the build in the tree and move
whenever the kernel does, so re-measure before trusting one:

	mxsize unix.link		text, data and bss totals
	mxnm unix.link			_ebss, blist, _ustack, _u, the slot
	od -An -tu1 -N16 x.o		one object's text/data/bss (obj.h)
	boot.c's nblk			(text + data + 0x10 + 511)/512

The *shape* is what is fixed: the u page has moved up to segment 15, so
the two adjacent pages are the buffer window (14) and the scratch window
(13) under it.

```
0x0000 - 0x0fff   ROM + I/O page            (not in the image)
0x1000 - 0x8895   resident text             30870 bytes
0x8896 - 0x8fff   pad                       the linker pads out to the slot
0x9000 - 0x9fff   overlay module slot       OVLSEG; ovlslot.o, one page
0xa000 - 0xcbf2   data + bss                11251 bytes: OVLDATA to _ebss
0xcbf3 - 0xdff6   buffer pool headers       244 minted headers off _ebss
0xdff7 - 0xdfff   free                      9 bytes
0xe000 - 0xef4a   window-page cluster       BUFSEG; win.o, 3915 bytes
0xef4b - 0xefff   free in the window page   181 bytes
0xf000 - 0xfb31   leaf code + constants     USERSEG; 2866 bytes
0xfb32 - 0xfd31   ustack                    512 bytes, .bss
0xfd32 - 0xfda6   struct user u             117 bytes, .bss
0xfda7 - 0xffff   free                      601 bytes; loader-stack slack
```

`blist` is the last object in .bss: the 8 boot headers at 0xcb4b..0xcbf3,
which is why the pool's minted range starts at `_ebss`.  The headers are
21 bytes (`sizeof(struct buf)`), so the pool is 252 of them — the 8 boot
headers plus 244 minted — 5292 bytes from 0xcb4b to `btop` = 0xdff7.

What binds the pool is the ceiling now, not the count: the room to
BUFWIN is 244 headers and `MAXBUFS` (256, main.c) is above it.  It was
the other way round — the ceiling used to leave room for 282 and the
count bound first — and the kernel's own growth is what closed the gap.
See docs/DRIVERS.md.  The 0xe000 page is the wall for the resident
kernel as well: `-Shigh` parks highmem.o (3254) at `_ebss` and win.o is
pinned at the window, so the kernel links only while
`_ebss <= 0xe000-3254` = 0xd34a.  Today's slack is 0xd34a-0xcbf3 = 1879
bytes, none of it reachable by the cache, and it is where the driver
blobs would have to park.  See TODO, the scsi section.

The image's own bytes run 0x1000 to 0xfb31 — the text and the slot page,
then the data at 0xa000, win.o at 0xe000 and the leaf code at 0xf000, the
bss and the pool in between carrying none — and the boot loader reads the
lot as one flat run: `boot.c` counts `(text + data + 0x10 + 511)/512` =
118 blocks from 0x0ff0, reaching 0xfbef.  `_usrtop`/`_memtop` = 0xffff
(uhdr.s).

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
buffer data.  The ceiling is what binds, not the count: the room to
0xe000 is 244 headers, and `MAXBUFS` (256, main.c, which caps the minted
count) is above it, so the pool is 8 boot headers plus 244 minted,
running 0xcb4b to 0xdff7, the 3254 bytes from `_ebss` being over the
reclaimed init-only object, and 9 bytes of the `_ebss`..BUFWIN region go
unused.  It is the reverse of what it once was — the ceiling used to
allow 282 and the count bound first — and the kernel's own growth is
what closed the gap.  Those 9 bytes are all the room there is: the next
header's worth of growth eats a buffer, at 21 bytes each.  See TODO, the
scsi section.

The blocks are 512 bytes each, minted `data = BUFWIN + (i&7)*512` with
`xmem` = a fresh page from `segalloc()` (8 blocks per page).  DMA
ignores the window: physical = `xmem<<12 | (data & 0xfff)`.

Raising the ceiling to `_upage` (0xf000) roughly doubles what memory
would allow, but `MAXBUFS` binds today and would have to go up with it;
and it needs the window borrowed at every site that uses it — see TODO.

A fork can then reuse a filled-out page: keep a freelist of u pages
that already carry the leaf code, copy only the 629 bytes of ustack and
u, and skip the text copy (see TODO).

## Constraints

- **Loader stack**: mwboot1.s parks sp at 0xffff and loads the kernel in
  512-byte blocks from 0x0ff0.  The image must not reach the top, or the
  last block wraps its DMA write into the rom and walks over the stack.
  This is a boot-time wrinkle, not an architectural brake — the loader's
  stack can move (backward-compatible: it reads the same header and
  loads contiguously), and the image the kernel links to now ends at
  0xfb31 — the last block the loader writes ends at 0xfbf0, so the
  margin to the loader's deepest sp (0xffde) is 1006 bytes.  What binds
  the leaf code today is the u page's own 4K, not the loader.
- **48K fit**: for the scratch window at 0xd000, text+data+bss must fit
  0x1000-0xcfff.  It ends at `_ebss` = 0xcbf3 today, 1037 bytes under the
  window.  Gated by the leaf-code move above shrinking the text.
- **.data, not .text**: leaf code in the u page (see above).
