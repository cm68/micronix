# Function-scope `static` cleanup

A tracked plan for removing the Whitesmiths-era function-scope `static`
declarations from the kernel. Scope is `sys/` only.

## Why (two reasons, one is a bug)

1. **Correctness.** A function-scope `static` is one storage cell shared by
   every process. Used as per-call scratch it is fine only while the function
   *and everything it calls* never sleeps; the moment a disk i/o sleeps, a
   second process enters, steps on the cell, and corrupts the first. This is
   already flagged at the top of `sys/TODO` ("declaring scalar types static
   when the compiler runs out of registers ... very very broken code ... if
   the function ... could sleep"). Converting scratch statics to locals gives
   every process its own stack copy and removes the hazard. This is the real
   reason to do the work; the size is a bonus.

2. **Size.** Resident text is at the 0x8000 budget ("0 headroom"); the statics
   also cost `.bss`/`.data` in four different places (see "where the bss
   pays" below).

## Why the statics are there (three distinct reasons)

- **A — Whitesmiths register-allocator workaround.** 16-bit/8-bit scalars and
  pointers declared `static` and used as scratch, because the old compiler
  would not keep them in registers. `ccc -O` now promotes these to IX/BC by
  reference count and emits `bit n,(ix+off)` / `ld r,(ix+off)`. **Obsolete —
  convert.**
- **B — 32-bit-local workaround.** `UINT32`/`long` statics (`lba`, `n`,
  `offset`, `phys`, `abs`, `a`). Still needed: "every access to a 32-bit local
  is a call" into the q* runtime (GNUmakefile:99). **Keep as statics** — moving
  them to locals is a text regression.
- **C — genuine persistent state.** Counters, reentrancy/busy flags, cursors,
  real buffers/arrays/structs. **Keep**, but audit that each is locked or
  provably non-sleeping (see checklist notes).

## Classification rule (per function)

Convert a static to a plain auto local only when **every read is preceded by a
write within the same call** and nothing must survive to the next invocation.
Drop `static`, add nothing else (no `register` — plain locals measured better:
access.c `register`=391 text vs plain=371). Keep it static if it fails that
test, or if it is `UINT32`/`long`, or an array/struct/buffer.

## Scope

386 `static` declarations in `sys/*.c` (excluding the six `*hdr.c` overlay
headers, which are the legitimate `struct ovlhdr`):

- **176 file-scope** — globals / static functions / cross-function interrupt
  state. Out of scope. `hdca.c` is all file-scope (40): its interrupt handler
  resumes a "phase" and the current op is shared, so those stay.
- **210 function-scope** — the work, across 31 files, split by linkage:

| linkage | files | fscope statics | where its data lives |
|---|---|---|---|
| resident `KERNEL_C` | balloc cio exec fio fork ialloc iget malloc name queue sleep subc swap time tty uio dlabel | ~84 | `_ebss` — feeds buffer pool |
| window `WIN_C` | access open sys2 rdwri memdev link | ~16 | `win.o` page (0xe000) |
| `HIGHMEM` (init-only) | cus djinit | ~3 | parked at `_ebss`, reclaimed |
| drivers `MODS`+`STAGED` | ide ncr dj mw disk2 disk3 | ~107 | each driver's 4K module page |

Within the 210, subtract the keeps:

- **15 × 32-bit** (reason B): ide×4, ncr×5, disk3×4, dj×1, sys2×1.
- **~8 persistent** (reason C): `pid` (fork), `count` (iget, uio), `execing`
  (exec), `tmbusy` (time), `circle` (sleep), `step`/`settle` (dj), `more`
  string (tty).
- **~5 arrays/structs** (reason C): `temp[12]` (sleep), `taskfile[IDELEN]`
  (ide), `buf[TTYHOG]` (tty), `struct dlabel lab` ×2 (dlabel).

Leaves **~180 convertible scratch statics**, concentrated in the drivers
(ide/ncr/dj/mw/disk2/disk3 ≈ 107 of them).

## Expected win

- **Text**: measured anchor access.c — 3 statics → 27 bytes text (398→371).
  Ceiling ~180 × ~9 B ≈ **~1.6 KB**, the prize (resident text has 0 headroom).
- **Bss/data**: the freed storage pays in a different currency per bucket —
  see below.

## Where the bss pays

The `.bss` is not one pool; it lands in four regions and buys four different
things (measured 2026-10-12, `unix`; the tree is in flux — `GNUmakefile` is
modified and `OVLSEG` has moved, so re-measure before relying on a figure):

- **resident `KERNEL_C`** → the buffer pool. `expand_bufs()` (main.c) mints
  `struct buf` headers from `&blist[8] == _ebss` up to `OVLBASE` (0xc000,
  `OVLSEG=12`), clamped by `MAXBUFS`=256. `struct buf` is now 19 bytes (the
  `count` field was dropped). Current `_ebss`=0xb116 → ~208 buffers, so the
  pool is **address-bound**: each ~19 bytes of resident bss freed mints one
  more buffer, saturating at the 256 cap (~900 more bytes to free).
- **drivers** → each driver's 4K module page (code+data together). `ncr.mod`
  is the tightest at 3822/4112; freeing driver statics buys module-page
  headroom, not buffers.
- **window `WIN_C`** → the `win.o` page, currently 4021/4096 (**75 bytes
  free**, the tightest data budget). access/open/sys2/rdwri/memdev/link bss
  buys `win.o` headroom, not buffers.
- **`HIGHMEM`** → parked at `_ebss` and reclaimed as buffer headers after init
  anyway, so its statics are effectively free lunch.

Net: only the ~84 resident statics turn into buffers; the ~107 driver statics
turn into module-page room for ncr; the ~16 window statics turn into win.o
room. Text stays the primary motive.

## Procedure (per function)

1. Confirm scratch (read-after-write within the call, nothing survives).
2. Drop `static` (and any redundant copy like `ip = i`).
3. `mxccc -m micronix -O -DOVLSEG=12 -DN8259=2 -i../include -s` and read the
   `.s`: confirm `bit n,(ix+off)` / `ld r,(ix+off)` and the symbol gone from
   `.bss`.
4. `mxsize` before/after: text and bss must both shrink; if text grows the
   function is skipped (allocator doesn't like it).
5. Per file: `make`, link, boot (hwsim harness), run the self-compile test
   (cross→native1→native2, native1 ≡ native2).

## Risks / gotchas

- **Register-allocator correctness.** ccc has known hazards (switch register
  hazard, callee-save BC/IX, DE-return frames). A smaller-but-wrong function is
  worse than a bigger correct one — every conversion gets the `.s` read and a
  boot test.
- **32-bit trap.** Never convert `UINT32`/`long` (q* calls).
- **Frame size.** Don't convert `struct dlabel lab` and friends; a big auto
  struct grows the frame and hits SP-relative-offset costing.
- **Overlay drivers.** Compiled twice (kernel + `.mod`/`.page`); each
  conversion must be checked against the module link (≤4112).
- **Window / u-page linkage.** Check the object's linkage class before touching
  it (access.c is `WIN_C`).

## Order

- **Batch 0 — canary:** access.c (analyzed; lock the measured loop).
- **Batch 1 — small core:** cio link subc balloc cus memdev rdwri fio open time.
- **Batch 2 — medium core (mixed keep):** name queue malloc swap iget uio
  ialloc sys2 exec sleep fork tty.
- **Batch 3 — drivers (biggest text win, one routine at a time):** mw ide ncr
  dj disk2 disk3 — keep every `UINT32`, verify each module page.
- **Out of scope:** hdca.c, the six `*hdr.c`, all file-scope globals.

## Checklist

### Batch 0
- [ ] access.c — access(): drop `ip/perm/mode`; permission(): drop `ip`, keep
      `euid/egid`? (verify: they are saved across the access() call, so locals)

### Batch 1
- [ ] cio.c — fp/ip/dev (verify scratch)
- [ ] link.c — p
- [ ] subc.c — c
- [ ] balloc.c — fb (143), i (273) (verify; fb may persist across calls)
- [ ] cus.c — p/top/b
- [ ] memdev.c — n, port
- [ ] rdwri.c — mode/nbytes
- [ ] fio.c — type (31), gap (33) [gap is long → KEEP]; type/dummy (85-86)
- [ ] open.c — writ (82), ip/writ (144-145)
- [ ] time.c — KEEP tmbusy (persistent)

### Batch 2
- [ ] name.c — phys/end/dummy/nents/c/p/nambuf/b/d, x/y/i
- [ ] queue.c — x/y, from/cleft/qspace/togo/nfill, to/cleft/nsent/togo, new/old, b, new/old
- [ ] malloc.c — i/m (96-97), i/grow/m (123-124), grow/i/m (148-149), i/grow/m (178-179), i (294)
- [ ] swap.c — p/n (66, 86), i/j/success/blk/b (162-164)
- [ ] iget.c — i (146); KEEP count (145)
- [ ] uio.c — b (273, 321, 490), p/f/bc/pc/fc/n (296-298), maj (475), dev (491); KEEP count (272)
- [ ] ialloc.c — bp/dp/ip, sb/sup
- [ ] sys2.c — fp (101); KEEP offset (102, UINT32)
- [ ] exec.c — brake/tbrake (145), s/d (165), n/c/s/a/d/av/ac/count/rem (202-203), sp/st (254), n (298); KEEP execing (57)
- [ ] sleep.c — i (147); KEEP temp[12] (117), circle (146)
- [ ] fork.c — bank (69); KEEP pid (131)
- [ ] tty.c — p/q (105, scratch halves); KEEP buf[TTYHOG] (105), more (328)

### Batch 3 (drivers — keep every UINT32/long)
- [ ] mw.c — info/drive/sl/b (190-192), b (294), sl/off/roll/len/maxblk (323), f (454),
      track/head/sector/csec/nsecs/curtrk (535-536), cyl (591), cp (861),
      info/p/i/hole (878-881)
- [ ] ide.c — KEEP n (519, 733), lba (789, 942), taskfile[] (1044); convert the rest
- [ ] ncr.c — KEEP n (486, 520, 598), lba (903, 1061); convert the rest
- [ ] dj.c — KEEP phys (653), step/settle (509); convert the rest
- [ ] disk2.c — convert (verify any UINT32)
- [ ] disk3.c — KEEP abs/a (349, 408, 470, 488); convert the rest

### Follow-ups (not part of this cleanup)
- [ ] Audit persistent statics for locking (itime/btime already `di()/ei()`;
      execing/tmbusy already sleep-guarded) — see sys/TODO.
- [ ] Update the memory notes: `kernel-memory-budgets` / `kernel-resident-space-is-full`
      are stale (`OVLBASE` 0x9000→0xc000, `struct buf` 21→19 after the count drop).
