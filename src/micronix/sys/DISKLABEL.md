# DISKLABEL — what a disk says about itself

A design note on three things a Micronix disk should be able to say without being
told: what geometry it has, where its filesystems are, and whether it has a boot.
The first is implemented and in the field. The other two are not, and the disks that
already exist keep mounting untouched either way.

`struct dlabel` (`include/sys/dlabel.h`) sits at `DL_OFFSET` in the boot sector —
`mkfs` writes it (`cmd/mkfs/mkfsfunc.c:169-208`) and both loaders read it
(`stand/boot/mwio.c:110-168`, `ideio.c:184-225`).

## The problem

A Micronix hard disk does not describe itself, and the geometry it needs is written
nowhere on it.  Everything — where the superblock is, where the boot area is, which
cylinder a block lands on — comes from tables compiled into three separate places:

| where | what | keyed by |
|---|---|---|
| `sys/mw.c:34-48` | `specs[]`, five rows: tracks/heads/sectors plus stepper timing | `minor >> 2` |
| `cmd/mkfs/mkfs.h` | `dtracks[]`, `dheads[]`, `dsecs[]` | its own `type` argument |
| `stand/boot/Makefile` | `DRIVES`, one row per drive the images are built for | the image's name |

Two programs that disagree about those tables do not fail.  They read and write
different cylinders while both reporting success, which is the worst possible
outcome and the reason the label exists at all.

There is exactly one place on the device that can be found knowing nothing about
it: **physical cylinder 0, head 0, sector 0** — cylinder/head/sector, "CHS", the
geometry a drive must be told before it can be asked for a particular sector.
That is the sector the boot ROM reads, so it is the only address that works
before anything is known.  The label goes in the second half of it, at
`DL_OFFSET` (256), so the boot code can have the first half.

Two things can say what a disk is, and they are not equal.  **The label is the disk
speaking.**  It is marked with a magic number and sits at a fixed address, so if it
is there it is believed and used — the kernel does not second-guess it against a
table.  **The minor number is the person speaking.**  It names a row of `specs[]`
(`sys/mw.c:34-48`), and that is the fallback, for a disk laid down by something that
wrote no label.  Neither is a reason to refuse on its own: a disk with a label mounts
from the label whatever the minor says, and a minor naming no row mounts from the
label alone.  Only a disk with neither is refused, because then there is no geometry
and no block number can be mapped at all.

The one asymmetry worth knowing is that the boot loader has no minor number to
consult — it is entered by the monitor with no device number — so `stand/boot/mwio.c`
refuses a label-less disk outright and says so.  A label-less disk can therefore be
mounted as a data disk but not booted from.

What the label does **not** carry is timing.  `specs[]` holds three numbers that
belong to the drive and not to the filesystem — `stpdel`, `precomp` and `lowcur`, the
step-pulse delay, the precompensation cylinder and the low-current cylinder
(`sys/mw.c:34-48`) — and the label has no field for any of them.  So the table never
retires even once every disk is labeled: a labeled disk still takes its timing from
the row the minor names, and from conservative defaults when it names none
(`sys/mw.c:233-244`).  The geometry is the label's and the timing is the table's, and
a geometry that disagrees with the row is worth warning about (`sys/mw.c:264-270`)
precisely because the timing has already been taken from it.

## The roll, and why it is the thing to fix

The mapping from a filesystem block number to a CHS address is `mwcyl`
(`sys/mw.c:618`):

    cyl = blk / spc + roll;
    if (cyl >= tracks) cyl -= tracks;

where `spc` is sectors per cylinder (`heads * spt`) and `roll` is `tracks >> 1` —
half the platter.  The roll exists so that **filesystem block 0 does not land on
cylinder 0**, because cylinder 0 belongs to the boot.  On the five-meg drive
(153 x 4 x 17 = 10404 blocks) filesystem block 0 is physical cylinder 76, and the
boot area sits at filesystem block `d_cyl0 = (tracks - roll) * spc` = 5236.

Three costs, all load-bearing today:

1. **The boot area is a hole in the middle of the filesystem.**  The free list has
   to be taught to skip it (`inboot()`, `cmd/mkfs/mkfsfunc.c:122,419`) and
   something has to own the blocks so no repair hands them out — that is
   `/boot/boot`, and `installboot()`, `bootrange()` and `icheck -i`/`fsck -i`
   exist to rebuild it.
2. **A filesystem cannot be larger than 32 MB.**  `s_fsize` is 16 bits
   (`include/sys/fs.h:17`), the buffer's block number is 16 bits (`UINT blk`,
   `include/sys/buf.h:10`), and `info->maxblk` — `tracks * heads * sectors` — is a
   `UINT` as well (`sys/mw.c:62,291`), so it wraps at 65536 blocks.  A 200 MB drive
   is four hundred thousand blocks and six times that limit; it cannot be addressed
   as one filesystem at all.
3. **The rotation can put the filesystem on top of the label.**  The wrap sends
   high logical blocks to low cylinders; at or near `roll == 0` the superblock and
   the ilist land on cylinder 0, the one sector the label occupies.

The IDE card makes all three urgent.  A 512 MB ATA drive is a million blocks,
addressed by **LBA** — linear block addressing, the whole disk numbered from 0,
which is what a drive with its own controller speaks, and why `ideboot1.s` needs
no geometry at all ("LBA 0, the drive's first").

## The slice table

A **slice** is a window on the drive: an offset and a length, in cylinders.

Eight of them, lettered a through h, which is the traditional count.  **They may
overlap**, and the driver validates nothing: a slice is two numbers and a window,
with no disjointness check, no ownership of a region, and no complaint if two of
them name the same cylinders.  Two filesystems over one region is the operator's
error and always was.

Two conventions come with the letters:

- **'c' spans the whole disk, including the label.**  It is the raw handle, and it
  is how a disk is relabeled: a program opens 'c', edits block 0's second half,
  and writes it back.  That is ordinary buffered I/O through the block device — no
  new syscall, no kernel provision, and no part of the disk the block device cannot
  address.  (The stub in `lib/fslib/fslib.c:113` says the label "cannot be reached"
  as a block; 'c' block 0 is that block.)
- **Block 0 is never part of a filesystem.**  Every slice reserves it, so a
  filesystem's superblock is at block 1 of whatever slice holds it.  On 'c' the
  reserved block is the label's, so the label is structurally outside every
  filesystem on the disk: `mkfs` on 'c' writes its superblock at block 1 and leaves
  block 0 alone.  The worst thing anyone can do to a disk cannot destroy the thing
  that tells you what it is.

The mapping a slice gives:

    cylinder   C = slice.offset + blk / spc + roll, wrapped at tracks
    sector/head from blk % spc

The offset is in **cylinders, and that is the point of it**.  A filesystem block is
16 bits, so a slice holds at most 64K blocks = 32 MB; but the slice's *position* is
added after the division, so the position never has to be a block number and never
has to fit in 16 bits.  A 200 MB drive is four hundred thousand blocks and 16 bits
cannot count them, yet a filesystem at its far end is `slice.offset + blk / spc`,
both terms small.  The block number, and the 32 MB ceiling that comes with the
buffer layer, is the price of the filesystem; the slice offset costs nothing at all.

## The boot is an attribute of the drive

Not every block device needs or wants a bootloader, so a boot is two statements
about the drive, and neither of them is a slice:

    d_bootblks      how many blocks it owns, from device block 0.  0 = no boot.
    d_bootslice     which slice it loads its filesystem from.

Device block 0 is the one address that needs no geometry, and it is where the first
level is and where the label is; the second level follows it at device blocks 1, 2,
3, read blindly by the same sector-increment loop that works today.  Nothing in a
filesystem is the boot: the root filesystem holds neither the boot nor the label in
band, and the boot's extent is a region below every slice.

**This is what retires the protection apparatus on a new disk.**  `inboot()`,
`/boot/boot`, `installboot()` and `bootrange()` exist because on a rolled disk the
boot is *inside* the filesystem and something must own its blocks.  With the boot
below the filesystem there is nothing inside a filesystem to own, and the protection
is structural rather than bookkeeping.  They all stay, as the v1 path; `bootrange()`'s
stub stops being a defect to fix and becomes a v1-only tool.

## The label

`struct dlabel` grows by appending, so a label written before a field existed reads
correctly with that field zero.  Existing fields keep their positions:

    d_magic[4]      "MWDL", readable in a hex dump
    d_version       of this structure; 1 = the roll idiom, 2 = the slice idiom

    d_tracks        cylinders
    d_heads         heads
    d_spt           sectors per track
    d_cyl0          the filesystem block at physical cylinder 0  (v1: where the boot is)
    d_bootblks      how many blocks the boot owns, from device block 0
    d_roll          what the mapping adds to blk / spc
    d_fsize         blocks in the filesystem
    d_isize         blocks of inodes
    d_swap          blocks left at the end, not in the filesystem

    d_bootslice     NEW: the slice the boot loads its filesystem from
    d_slice[8]      NEW: offset and length, in cylinders, per slice a..h

Appending is safe, and the reason is worth stating because the compatibility argument
rests on it: `mkbootimg` builds the boot sector in a `static` buffer
(`stand/boot/mkbootimg.c:46`), so the half past the fields it writes is zero, and it
writes all 512 bytes.  `mkfs`'s `putlabel` then overwrites the fields in a buffer read
back from that image and writes the block whole.  So on every volume that exists, the
bytes a new field would occupy are zero.

**A v1 disk is a v2 disk whose table is all zero.**  An all-zero table has no other
reading — a zero-length slice names nothing — so a reader takes it as one slice:
offset 0, the whole disk.  That is exactly the rolled layout, with `d_roll` doing the
rotation as it does today.  So neither the kernel nor the loader branches on
`d_version`; the generation is in the values, and one formula covers both:

| | slices | `d_roll` | boot |
|---|---|---|---|
| **v1**, made by the code today | none (read as offset 0, whole disk) | `tracks >> 1` | inside the filesystem, at `d_cyl0`, owned by an inode |
| **v2** | the table | 0 | below the filesystem, at device blocks `0 .. d_bootblks-1` |

`d_roll` therefore stays a first-class field.  It is not vestigial and it is not
deleted — it is how a rolled disk describes itself, and with the table reading as
offset 0 the formula is byte-for-byte today's mapping, so **an old rolled volume
mounts with no relabeling and no preparation of any kind.  Its label already says
what it is.**

`d_version` is for the *tools*, which must know whether to write a boot inode; the
kernel and the loader read values only.  `d_offset` was the first cut at what the
table now does — one filesystem's offset — and it retires: the table says where a
filesystem starts, and a v1 disk's table says 0.

## The loader

`stand/boot` is where the boot attributes are read, and it costs almost nothing.

`mwio.c`'s `reset()` already reads device block 0 into `0x1000` — the one address
that needs no geometry — and takes the label from `0x1000 + DL_OFFSET`
(`mwio.c:135,146`).  So the slice table and `d_bootslice` are in memory when they are
wanted: no extra read and no new addressing.  What the loader needs out of them is
one number, the boot slice's offset in cylinders, kept in `spec`; and it has to be
taken in `reset()`, because the label's memory is `buf0`, which `boot.c`'s first
`readblock` overwrites.

`readblock` (`mwio.c:209`) carries the kernel's mapping verbatim:

    cyl = blocknum / spec.spc + spec.roll;

and it becomes `spec.cylstart + blocknum / spec.spc + spec.roll`.  Everything the
second level reads is a filesystem block — the ilist at block 2 (`boot.c:272`), the
directory and indirect blocks out of `d_addr[0]` (`boot.c:199,246`) — so one number
added after one division is the whole of it.  It does not read the superblock at all;
`boot.c` hardcodes the v6 layout and never touches `s_isize` or `s_fsize`.  The
requirement is the same either way: it must know where the filesystem begins, and
that is the one thing only the label can say.

Two of the loader's own fields are too small for the drive this work exists for:
`struct drivespec` holds `UINT8 spc` (`mwio.c:13`), and `spc` is `heads * spt` — 1008
on a 16-head/63-sector ATA drive, which truncates.  `spec.limit` is
`d_tracks * spc - 1` in a `UINT`.  Both are fine on every drive in the tree and
neither is fine on a 200 MB MFM drive or a 512 MB IDE one.

## The ceiling

A slice's size is bounded by the *filesystem*, not by the label: 64K blocks, 32 MB.
`blk` is a `UINT` (`include/sys/buf.h:10`), `s_fsize` is a `UINT`
(`include/sys/fs.h:17`), and `info->maxblk` is a `UINT` (`sys/mw.c:62`), so a
filesystem — and therefore a slice — tops out at 32 MB however long the table says it
is.

The drive is bounded by the geometry field, which is a cylinder: 16 bits is 65535
cylinders, so 2.28 GB at the tree's five-meg geometry (68 blocks to a cylinder),
3.42 GB at the m16's (102), and 33.8 GB at ATA's practical maximum of 16 heads and 63
sectors (1008).  Eight slices of 32 MB is 256 MB reachable at once — enough for
everything the tree has ever had, and the reason a 200 MB drive is used as four or
eight filesystems rather than one.

## A disk that would need it: the Wren II

Notional, because none of it is on real hardware: a CDC 94155-86 — the Wren II — is 925
cylinders and 9 heads, and the Morrow format is 17 sectors of 512 bytes to a track, so a
cylinder is 153 blocks and the drive is 141,525 blocks, 72.5 MB.

The wall lands at 428 cylinders.  428 × 153 = 65,484 blocks, which fits in a 16-bit block
number with 51 to spare; 429 × 153 = 65,637, which does not.  A 429-cylinder slice is
legal and its bound clamps at 65,535, so its last 101 blocks — two thirds of a cylinder —
can never be named by a filesystem.  That is the ceiling above, in the only form an
operator meets it in.

So the drive is three filesystems and the boot:

	a	(1, 428)	65,484 blocks	the whole 32 MB a filesystem can hold
	b	(429, 428)	65,484 blocks	same
	d	(857, 68)	10,404 blocks	the leftover
	c	(0, 925)	the whole disk, by convention

1 + 428 + 428 + 68 = 925 cylinders, and 153 + 65,484 + 65,484 + 10,404 = 141,525 blocks:
the arithmetic closes, and cylinder 0 — the label and the boot — is the only thing on the
drive outside a filesystem.  That is what the slice model buys and the roll could not:
nothing maps to cylinder 0, so there is no hole in the middle of a filesystem to reserve,
no inode to own it, and nothing for a repair to hand out.  `d_roll` is 0, `d_cyl0` has no
meaning, `d_bootblks` is 153 — one cylinder — and `d_bootslice` is 0, slice 'a'.

Each slice carries its own superblock at block 1 and its own ilist: `fsize` is the slice's
block count, and `mkfs` sizes the ilist `fsize / 43 + fsize / 1000`
(`cmd/mkfs/mkfs.c:209`) — 1,587 blocks for 'a', 251 for 'd'.  The same formula gives the
m16's 756, which is what a fresh volume's label reads back, so the numbers are checkable
rather than asserted.

Two things the example makes concrete, and both are already on the list:

	The loader reads `d_roll` from the label but not the table, so for this drive it
	would look for block 0 of slice 'a' at cylinder 0 and find the boot area instead.
	Until `spec.cylstart` lands in `mwio.c`/`ideio.c`, a filesystem at an offset
	cannot boot — which is what the tests below show happening.

	9 heads does not fit `(~head & 7)`: head 8 wants a fourth bit, and that bit is the
	wire the controller uses for low current.  A notional format is fine — the
	simulator does not model the cable — but the real drive waits on `sys/TODO`.

The letters left over, e through h, are not *absent*: an unwritten entry is (0, 0), which
reads as the whole disk, so those letters alias the drive.  Harmless, and the price of the
v1 compatibility that makes an empty table mean the rolled layout.

## Open decisions

1. ~~**Where the three slice bits go in the minor number.**~~ **Decided and in:** the high
   bits, 5-7.  `minor(dev)` is eight bits (`include/sys/con.h`), and `devslice(dev)` is
   `(minor(dev) >> 5) & 7` beside a `devtype(dev)` of `(minor(dev) >> 2) & 7`.  Stacking
   them that way keeps every existing node meaning exactly what it means today, with the
   slice defaulting to 'a' — `m5a` 0, `m10a` 4, `m16a` 8, `ide0` 0 — where putting the
   slice in the middle would make `m5b` (minor 1) change from "drive 1" to "drive 0, slice
   b" and misaddress the `/dev` on every disk already in the field.  **The price, which is
   the part worth knowing:** the type field shrinks from six bits to three, so a driver's
   geometry table is eight rows rather than sixty four.  `specs[]` holds five and
   `boards[]` one, so nothing is displaced — but a sixth through eighth drive size is the
   last this encoding can take, and `type = minor >> 2` is gone from both drivers.
2. **The names.**  The drive letter and the slice letter collide: `dev/devlist` has
   `m5a` through `m5d` as four *drives*, not four slices.  They need different
   alphabets, or a different arrangement.
3. **Whether `d_bootblks` is needed at all**, or the boot's extent is simply the
   operator's layout and nothing records it.  Its case is the labeler, which wants to
   know where it may start a slice.
4. **Whether a slice longer than 32 MB is refused, clamped, or allowed** with the
   filesystem's own limit doing the work.  The driver as built takes the third: the slice
   is allowed and the *bound* is clamped at `0xffff` blocks, since a filesystem longer
   than 64K blocks cannot be addressed anyway, so a longer slice only means the tail of it
   is unreachable — not that the request is illegal.  Refusing would be the other
   defensible answer and would make an oversized slice a loud mistake; the argument
   against it is that a whole-disk slice ('c', or an empty table) is *always* oversized on
   a large drive, and a v1 disk's table is empty, so refusing would break every drive the
   work exists for.
5. **Whether `d_swap` retires into slice 'b'**, which is what 'b' is for by
   convention.

## What it would take

1. **Label, table and mapping.**  **Done.**  `d_bootslice` and `struct dslice d_slice[8]`
   are appended to `struct dlabel`, `d_offset` is retired (nothing ever wrote it), and
   `devslice()`/`devtype()` decode the minor.  `mwopen`/`ideopen` read the slice the minor
   names and take `cylstart` and the slice's extent from it; `mwcyl`/`idecyl` add the
   offset after the division.  Two details the shape forced.  The bounds check is no longer
   a subtraction of an offset but the slice's own extent, and the extent is *clamped* at
   `0xffff` rather than allowed to wrap, with the multiply guarded by a division
   (`ncyl > 0xffff / spc`) so it is the clamp that cannot overflow — a slice longer than a
   block number can count has no blocks to lose, since 64K is all a filesystem can use.
   And a table naming more cylinders than the drive has is refused with `ENXIO`, which is
   what replaces the old "the offset is past the end" check: wrapping it would silently
   map the slice onto the front of the drive.  The offset is a no-op when the table is all
   zero, so an untouched v1 volume still mounts — verified booting both ways below.
   One loose end the mapping left: `mwclose` parks the heads with
   `bread(info->spc * info->roll, dev)` (`sys/mw.c:356`), which is the block that maps to
   cylinder 0 only while `cylstart` is 0 — it reads `roll` and not the slice.  The general
   form is `(tracks - cylstart - roll) % tracks * spc`.  On a sliced disk it parks at the
   slice's first cylinder instead of 0, which is harmless for a landing zone but is not
   what the line says, and `ideclose` has no park at all.
2. **The loader.**  `spec.cylstart` out of the label in `reset()`, the offset in
   `readblock`, and the two geometry fields widened.  **This one now blocks something
   concrete:** a slice-relative root filesystem cannot boot without it, demonstrated
   below.
3. **The tools.**  A labeler, for a disk with no label; `mkfs` reading the label
   instead of `mkfs.h`'s geometry arrays; `mkbootimg` losing its per-drive images.
4. **Docs.**  This file; `filesystem.5`, `mkfs.1`, `src/hwsim/DISKS`,
   `disks/hdinstall/REGEN`.

### How it would be checked

- **v1 compatibility, which is the point:** boot a volume made by the *current* code,
  untouched, and reach a shell.  No relabeling.  **Verified 2026-09-25**, on both paths:
  `root dev: hddma/8` and `root dev: ide/0`, each from a `create_vol`-style volume whose
  label is a v1 one — magic, version 1, geometry, `d_roll` 153, and the boot slice and
  the whole table reading zero, dumped with `od` at the label's offset to be sure.
- **The kernel's half of v2, by hand-labeling a v1 volume.  Verified 2026-09-25**, since
  nothing writes a slice table yet: `create_vol` a volume, copy it, patch the label's
  bytes at 2048+256 with `dd`/`python3`, point root at a slice (`setdev` 3/minor, where
  the minor is `slice << 5 | 8`) and boot.  What each run showed, all with `d_roll` left
  at the 153 the loader needs so that loader and kernel behaviour stay separable:
  - **the slice index comes from the minor.**  Root at minor 40 (slice 1) with slice 1 =
    `(0, 306)` boots — `root dev: hddma/40` — and it still boots with *every other slice
    set to offset 306*, which `mwopen` refuses.  So the kernel used slice 1 and only
    slice 1.
  - **the offset is added.**  The same volume with slice 1 = `(153, 0)` gets the loader
    through `Loading`/`Entering` and then never reaches a shell: block 0 maps to cylinder
    306, wraps to 0, and the kernel reads the boot area as its filesystem.  Offset 0
    boots and offset 153 does not, with nothing else changed.
  - **an offset at or past the end is refused, not wrapped.**  Slice 1 = `(306, 0)` stops
    at the root open — the log ends at `swap dev:`, before the superblock is read — where
    the `(153, 0)` run got past it.  Wrapping would have mapped the slice onto the front
    of the drive and booted it.
  - **the bound is the slice's extent.**  Slice 1 = `(0, 1)` — `maxblk` 101 — opens root
    and reads the superblock and the first inodes, then fails at the first block past
    101, where `(0, 306)` reaches a shell.
  - **what this leaves: the loader.**  A volume relabeled with `d_roll` 0 and slice 'a'
    = `(153, 0)` — arithmetically the v1 mapping exactly — does not boot at all: the
    loader takes `spec.roll` from the label (`mwio.c:146-154`) and reads no slice offset,
    so it reads the wrong cylinders and reports `Block out of range / inode read failed
    ... No bootable files`.  That is the loader item below, demonstrated rather than
    assumed.  The kernel-side runs above work around it by leaving `d_roll` at the
    layout the filesystem is really at.
  - **and one thing to look at:** both failure modes end in a hang at the mount rather
    than a message, so the observable is *where* the log stops.  Whether an unmountable
    root should say so is a kernel question this work did not answer.
- **v2 end to end:** label a blank volume, `mkfs` a slice, boot it.  The superblock is at
  the boot slice's block 1, checkable with a raw read at the expected cylinder.  It needs
  the loader item first, so it is the check the Wren II layout above cannot pass yet.
- **A slice past the 32 MB wall:** put a filesystem at the far end of a large drive
  and confirm the cylinder it lands on — the case a block-number offset could not
  reach, because the offset itself would not fit in 16 bits.
- **Relabel through 'c':** read and write block 0 of slice 'c' from the guest, and
  confirm the change takes effect on the next boot.
- **The roll's three costs:** no volume at any geometry has the label and the
  superblock on the same cylinder; a small filesystem on a big drive leaves the rest
  of the drive free; a table naming more cylinders than the drive has is refused
  clearly, not wrapped.
- **IDE:** boot `-b ../../micronix/stand/roms/mon500.bin -B ide` to `root dev: ide/0`.
- **A pre-label volume** mounts from the minor number when that names a `specs[]` row,
  and is refused loudly only when it names none.

<!-- vim: set tabstop=4 shiftwidth=4 expandtab: -->
