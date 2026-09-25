# ZEROCOPY — shared text via the buffer cache

How exec and fork stop paying for text, by mapping the buffer cache's own
segments into the process page table instead of copying text into fresh
segments.  This is a design note; nothing here is implemented.

The point: a Micronix text image is already resident in the buffer cache
(it was read to exec it, and the cache keeps it).  If a process's text
pages could *be* those buffer segments — mapped, not copied — then the
first exec pays the disk read (unchanged) and pins the pages, every later
exec of the same binary maps the same segments, and fork shares them too.
The cost per process drops from "N segments + N page copies" to "N
page-table entries + one refcount".

## The alignment invariant

For a buffer segment to map 1:1 as a text page, the file's text pages and
the process's text pages must be congruent at 4K granularity.

Today they are not.  The object header is 16 bytes (`include/obj.h`), so
text sits at file offset 16, while the assembler origins text at 0
(`ld.c` writes textoff=0, dataoff=text_size).  Off by 16.

Fix: origin text at 16 and put the header at memory address 0..15.  Then
file page N == memory page N for every N:

    file:     [hdr 16][text ................][data ................]
    memory:   [hdr 16][text ................][data ................]

Page 0 is header + the first 4080 bytes of text — all immutable, shared
read-only.  The header bytes mapped at 0..15 are harmless; nothing jumps
there, since entry is `u.pc = hdr.textoff` (16).

The alternative is to pad the header out to 4K and origin text at 0x1000,
making page 0 pure text.  Cleaner page 0, but 4K wasted per binary.

## The impedance mismatch: segments vs buffers

The page table is 4K-segment-granular (`mem[].seg` maps a whole segment or
nothing); the buffer cache is 512-byte-block-granular.  There is no
invariant tying a file's blocks to segment alignment: `expand_bufs()`
(`main.c`) mints segments in 8-header groups, but those groups are a
free-list of independent slots and `bread()` fills them one block at a
time.  A 5-page text image is 40 blocks that scatter across up to 40
segments.

The resolution is a *read policy*, not new hardware: read text as 8-block
runs, one segment per page, in page order.  That is the only genuinely new
mechanism.

## Disk blocks are not contiguous, so full buf headers

A text page is not a disk object; it is eight independent disk objects
that happen to be adjacent in memory.  Files allocate blocks through
`balloc()` with no contiguity guarantee, so each block needs its own full
`struct buf` — own `blk`, own `dev`, own DMA, own `BDONE`/`BERROR`.  There
is no page-level descriptor that can stand in.

The header is where disk and memory meet: `blk` is the disk address,
`xmem` + `data` is the memory address (`physical = xmem<<12 | (data & 0xfff)`,
per `KMEM.md`).  Page alignment is achieved by choosing which eight
headers' slots hold the page's blocks, not by any separate structure.

Consequence: a text page is eight block I/Os no matter what.  The disk
cost is identical to today.  The entire win is on the memory side — no
per-process segment allocation, no per-process copy.

## Headers are the scarce resource

The header-per-block model does not scale.  A cache covering the whole
pool — ~960K / 512 = ~1920 blocks — needs ~1920 headers, ~40K of kernel
address space, against a pool of 256.  Segments are plentiful (256);
headers are the binding constraint, and the cache is deliberately ~64K
(128 blocks) for exactly this reason (`KMEM.md`).  Today that 256 is
`MAXBUFS` in main.c — a count chosen, not a space measured — but it is
nearly the space too: the headers reach the 0xe000 window page within
three of that count, so the kernel has no room to raise it.

Shared text must not pin eight headers per page.  Split the header's two
jobs and persist only what is scarce:

- **read** — non-contiguous blocks force full per-block headers (own
  `blk`, own DMA).  These are transient: one header reused across the
  eight reads, or eight scratch headers freed once the page is in.
- **persist** — after the read the page is a coherent 4K segment; the
  text image needs only "page i -> segment S" (two bytes per page), not
  eight headers.

That keeps text out of the header budget entirely, at the cost of the
text segments leaving the cache: they are owned by the text-image table,
and on last-exit they return to `segalloc`, not the block free-list.  The
alternatives are a page-granular "clustered" header — one header holding
a page's eight `blk`s, cache-resident but a strategy-path change — or
moving the headers themselves into pool segments (VM), so the whole cache
scales.  Open decision.

## The COW boundary page

Data is contiguous with text (`dataoff = textoff + text`) and is not
page-aligned, so the page straddling the text/data boundary holds
immutable text *and* writable data.  A logical page maps to exactly one
physical segment, so it cannot be split: the boundary page must be shared
read-only until first write, then copied whole to a private segment.  That
is the one page that needs real copy-on-write.

Exactly one such page exists per binary — page 0 is header+text (all
immutable) and bss is private anyway.  The COW fires once, on the first
store into initialized data.  If text happens to end on a page boundary,
there is no boundary page at all.

The existing GROW mechanism is **not** usable here — its "copy-on-write"
never copies a byte (it shares a garbage segment and hands the writer that
segment on fault), and it uses the *deferred* fault model, which is wrong
for shared real data (see "COW: the fault model" below).  The boundary
page needs a real, precise fault handler — `segalloc` + `segcopy` + remap
+ refcount decrement.  `segcopy` already exists (`mem.s`).

## Lifecycle

A text image is a `(dev, inode)`-keyed table entry holding N segments and
a reference count, plus the pinned buffer headers.

- **exec** — look up `(dev, inode)`.  If absent, read the text as 8-block
  runs into N segments, pin the headers (`BLOCK`), record the image.  Map
  the segments into `mem[]` with the shared permission; the boundary page
  is mapped COW.
- **fork** — share the mapping, refcount++.  `bankcopy()` (`malloc.c`)
  currently `segcopy`s every FULL page including text; it must skip shared
  text and bump the count instead.
- **exit** — refcount--.  On zero, drop the image: if the pages stayed
  cache-resident (persistent headers), clear `BLOCK` and let them recycle
  through the free-list; if they were owned by the text table (transient
  headers), return the segments to `segalloc`.  See "Headers are the
  scarce resource".
- **swap** — text is never swapped; it is already on disk.  `swapio()`
  writes every FULL page today; it must skip shared text and re-map it on
  swapin.

Single-ownership is assumed in four places that all break: `mfree()`
(segs free all 17), `mrelse()` (frees the old image), `swapio()` (writes
every FULL page), and `bankcopy()` (copies every FULL page).  Each needs a
guard for the shared-text case.

## COW: the fault model (precise vs deferred)

The MPZ80 does real copy-on-write, and it has both fault models; the two
are not interchangeable:

- **deferred** (GROW, today) — the access completes, then the trap fires,
  and the instruction resumes at `pc+1` (`MAPBUG.md`).  Valid only because
  a grow page is garbage: the write landing on the shared segment is
  harmless, since the writer then owns it and everyone else gets a fresh
  segment.
- **precise** (real COW) — the write is blocked before any byte is
  modified, the trap fires, the page is copied, and the instruction
  restarts at `pc`.  The write never touches the shared page.

Shared text needs the precise model: the boundary page holds data other
processes still rely on, so a completed write cannot be discarded the way
a grow page can.

Both models are available, selected by R10 (RI10, `attr & 0x4`) in the
page's permission byte: with R10 set the access completes and then traps
(the GROW convention, resume at pc+1); with R10 clear a faulting access
is *inhibited* — blocked before it lands — and the instruction restarts
at pc.  So precise COW is just "map the shared page with R10 clear":
`fault()` copies the page, and the restart re-executes the write against
the private copy.  The deferred model stays reserved for grow pages,
where write-landing-first is harmless because the page is garbage.

The one awkward instruction is the indivisible read-modify-write
`ex (sp),hl`, whose 16-bit access spans a page when SP sits at 0xXFFF and
whose inhibited-write restart is the corner the hardware is least sure
about.  It is ruled out by the interface contract: the toolchain and
libraries are specified never to emit `ex (sp),hl` — a reserved
instruction, like the callee-save register and `csv` frame conventions —
so `fault()` can rely on its absence rather than treat it as a courtesy.

The kernel currently uses only `NONE(0)` / `FULL(3)` / `GROW(7)`.  Read
and execute are the same kind of access to the MMU: neither modifies the
page nor dirties it, so they are interchangeable here and the only axis
that matters is writable vs not.  Shared text — pure text and the
boundary page alike — is mapped non-writable: read and execute succeed,
and a write is the sole faulting case, inhibited (R10 clear), precise,
restartable.  A write to pure text faults; a write to the boundary page
COW's the page into a private `FULL` copy.  One new `mem[].per` code
marks "shared, non-writable", distinct from `FULL`.

## What exists / what is new

    exists   segcopy (4K LDIR), the BLOCK pin, per-block (dev,blk) bread,
             bmap/nread, the 8-per-segment header minting, newmap().
    new      text alignment (ld text_base = 16), the clustered 8-block
             read, the (dev,inode) text-image table + refcount, a real
             precise copy-on-fault path, the four single-ownership
             guards, and a header-management decision (transient vs
             persistent vs clustered).

## Cost model

    first exec   N*8 block I/Os (unchanged) + pin.
    later exec   0 I/O, 0 segments, 0 copy — N page-table entries, refcount++.
    fork         0 text copy (vs N segcopy today).
    boundary     1 segcopy per process, once, on the first data store.
