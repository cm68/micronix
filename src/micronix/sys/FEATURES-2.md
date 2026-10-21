# Micronix 2.x: functional extensions over 1.6

What this kernel does that the 1.6 kernel did not.  The baseline is the
1.61 kernel as shipped - `kernels/micronix.161` with its symbol table,
and the 1.6 sources preserved in
`extra/ORIGINAL/BU12/lens.m16/usr/src/sys.dir.Zarc.d` - against this
tree's `sys/`.  (Paths here are from the repository root, the directory
holding `src/`, `disks/` and `kernels/`.)  Where a count is given it was
read out of those two kernels, not guessed: the 1.6 syscall table and
buffer arithmetic come from `micronix.161` and its `.sym`, the 2.x
numbers from the build in this tree.

The extensions fall into four groups: system calls, caching, process
and exec performance, and drivers.  Each section names the files that
carry the change.

## Added system calls

The 1.61 kernel's `syssw` table has **49 entries** (0 through 48) and
`ncalls = 49`.  This tree's table has **57** (0 through 56).  Eight
slots are new (49-56): three are implemented and five are reserved.
Two old slots (45 and 46) also change from `badcall` to real calls,
making five implemented additions in all:

| # | 1.61 | 2.x | what it does |
|---|---|---|---|
| 45 | badcall | `dup2(fd, fd2)` | duplicate onto a chosen descriptor, closing the target first; `dup2(fd, fd)` is a successful no-op |
| 46 | badcall | `ftruncate(fd, length)` | truncate an open file to a 32-bit length |
| 54 | - | `ioctl(fd, cmd, arg)` | pass a raw command block to a block device's driver |
| 55 | - | `reboot(howto)` | flush inodes and dirty buffers until the cache is clean, then halt |
| 56 | - | `execve(name, argv, envp)` | exec with an environment |

The other five new slots are reserved: 49 and 50 are the lock/unlock
pair that a later 1.6 change described but the 1.61 image never
carried, and 51-53 are padding.  All five are `badcall` here -
`lock.c` is not in the kernel.

The calls themselves:

- **dup2** (sys/sys1.c).  The old `dup` could only take the next free
  descriptor.  A shell redirecting onto 0, 1 or 2 needs to name the
  target; `dup2` closes it if open, installs the file, and returns the
  new number.  The shell and `shim.c` use it for redirection.
- **ftruncate** (sys/sys1.c, `r_ftruncate` in sys/reg.c).  The length
  is 32 bits and arrives as two words, because the v6 register
  convention has only one.  It is what lets a program shorten a file
  in place instead of copying it.
- **ioctl** (sys/uio.c `bioctl`, sys/reg.c `r_ioctl`).  Not the
  terminal one: `stty` and `gtty` still do that.  `ioctl` carries a
  command block (`struct cdb`, `CDBCMD`) to a block driver - a SCSI
  CDB, an ATA task file, whatever the bus wants - and the kernel moves
  the data phase through one cache block when it fits.  A driver that
  declares no ioctl entry answers `ENOTTY`.  It is what makes
  `hdformat`, `mwformat`, `scsiformat` and the partition readers
  possible without a syscall per controller.
- **reboot** (sys/sys1.c).  The graceful path drains the cache through
  `isync()` and `bdrain()` until `ndirty()` is zero, printing the count
  each pass, and then halts.  Autoboot is not implemented; `RB_HALT`
  is the only path that currently exists.
- **execve** (sys/exec.c).  The 1.6 `exec` passed only argv;
  `execve` adds envp and builds both strings onto the new image's
  stack.  This is what lets the Bourne shell carry an environment.

The other entries are the 1.6 calls under their old numbers: `sleep`
is called `snooze` here and `permission` is the same call as access.
`times` (43), `profil` (44) and `ptrace` (26) remain unimplemented, as
in 1.6.

## Increased caching

The buffer cache is the largest single change, both in size and in
where it lives.

**1.6.**  `main.c` declared `blist[4]` and a `char (*buffer)[512]`
array; `binit()` expanded the headers and the data into one contiguous
region above `blist`, inside the kernel's own memory.  The 1.61
binary's `binit` computes

    nbuf = (0xffff - 0xe033) / (512 + 19) = 15

so the shipped kernel had **15 buffers**, headers and data both in its
high memory, ending just below the stack.  The data was addressed
directly: a driver wrote through `b->data`.  `bget()` was a linear scan
of all 15 headers, looking for the block and the least-recently-used
free buffer in the same pass.

**2.x.**  The two halves are separate.  The headers are still ordinary
kernel memory, 19 bytes each, but the 512-byte data blocks live in
unmapped physical segments and are reached through the buffer window
at 0xe000 (`bhold()`/`brel()`).  `bget()` hashes `(dev, blk)` into 64
buckets, so finding a cached block is a bucket walk rather than a scan
of the pool; the LRU scan remains for a miss.  `expand_bufs()` mints
headers from `_ebss` up to `SCRWIN` (0xc000), one 4K segment of eight
blocks per group of eight headers.

The result, at the time of writing (`_ebss` = 0xb669):

| | 1.61 | 2.x |
|---|---|---|
| buffers | 15 | **137** (8 boot + 129 minted) |
| cache data | 7.5K, in kernel memory | 68.5K, in segments above 16 |
| lookup | linear over the pool | 64-bucket hash on (dev, blk) |
| ceiling | top of kernel memory | `SCRWIN`, capped at `MAXBUFS` 256 |

The size is a function of the image, not a constant: every 19 bytes of
resident growth below 0xc000 costs one header.  The pool can be
clamped to 32 with `make DEBUGBUFS=1` in `sys` when a cache bug has to
show itself quickly.

The split buys the space twice.  The data no longer competes with the
kernel's text and stack, and the region the 1.6 pool used for data now
only holds headers - the rest of the space is available to the kernel.
`win.o` (open, sys2, rdwri, memdev, link, access) is linked into the
buffer page itself, because that code runs only while no hold is up;
its 4,012 bytes of resident text would otherwise have been 211 more
headers.

## Performance

### Task switching

1.6 had **one user task slot**: its `newmap` copied the process's
32-byte map into the task-1 map table at 0x620, and it ran on every
process switch.  The 2.x kernel has **fifteen user task slots**, one
per process, each with its own map table in map RAM, and `proc.task`
records which slot a process holds.

The map is still copied when it changes (`taskload()`), but not on
every switch.  `newmap()` runs at each switch and does the minimum:
allocate a slot if the process has none, copy the map only if it was
just allocated, write task 0's one u-page register to the incoming
process's u page, set `curmem`, and put the process's driver page back.
A steady-state switch therefore costs a few stores rather than a
32-byte map copy.

When all fifteen slots are busy, `taskalloc()` takes one from the
process least likely to want it back - an asleep process, the
longest-resident first - and the loser is reloaded from `mem[]` the
next time it runs.  The process table holds 20 entries, so this is a
cache, not a limit.

The u page is part of the same story.  It is a separate segment per
process, mapped into task 0 at 0xf000, and it carries the kernel stack,
`struct user`, and the leaf code that the kernel runs with the process
switched in.  Switching processes is switching stacks; nothing is
copied.

### exec

The 1.6 `exec` had `#define NBLKS 4`: it read the argument strings
into up to four cache buffers (`bget(nblks, NODEV)`), which capped the
arguments at 2048 bytes and held 4 of the 15 buffers for the duration
of the exec.  The block was then copied a second time onto the user
stack.

The 2.x `exec` builds the argument block **directly into the new
image's segments**, through the buffer window, using a fake buffer
header so `bhold()` can map a stack page.  A first pass scans the
vectors and strings for their lengths; the second copies them into the
segments being built.  No cache block is taken, and the limit is the
stack, not four blocks: `argsize` may run to 0xEFFF bytes (about 60K).
`execve` adds the environment to the same block.

The gain is both latency - two passes over small data instead of a
cache round trip and a second copy - and capacity: an exec on a full
cache no longer needs four free buffers, and arguments are no longer
bounded at 2K.

### The compiler

The 1.6 system was built with the Whitesmiths C compiler, which lived
on the distribution disks and was not part of the kernel tree; the 1.6
sources still read in that dialect (`fast`, `uns`, K&R declarations).

This tree carries its own compiler.  `cmd/ccc` is the driver,
`libexec/pass0` the preprocessor, `libexec/c0` the parser,
`libexec/c1` the code generator, and `libexec/peep` the peephole
pass that `-O` runs; `cmd/asz` is the assembler, `cmd/ld` the linker
and `cmd/ar` the librarian.  The host keeps `mxccc`, built with the
host's own compiler, and `mxccc` cross-builds the tree; inside the
simulator `ccc` runs the same passes and rebuilds the compiler and the
system there.  The self-compile check compares the native image
against the cross-built one.

Two properties matter to the kernel:

- **-O is a peephole pass over c1's output.**  It is not optional for
  the compiler itself: `c0` was failing to compile `declare.c` by less
  than a kilobyte, and -O takes 829 bytes off it.  The kernel and the
  tree's commands are built with it.
- **The archives are V7 and deterministic.**  `ar` writes fixed
  header fields (date 0, uid/gid 0, mode 0444), so an archive built on
  the host and one built inside micronix are byte-identical.  That is
  what makes the native build a check on the cross build rather than a
  second, different system.

### Drivers

The 1.61 kernel's block switch has **two real drivers** and one empty
slot:

| major | 1.61 | 2.x |
|---|---|---|
| 1 | nodev (the HDCA slot) | `hdca` |
| 2 | `djdma` | `djdma` |
| 3 | `hddma` | `hddma` |
| 4 | - | `ide` (ATA) |
| 5 | - | `ncr5380` (SCSI) |
| 6 | - | `nd` (network disk) |
| 7 | - | `disk2` (CompuPro Disk 2) |
| 8 | - | `disk3` (CompuPro Disk 3) |
| 9 | - | `disk1` (CompuPro Disk 1A) |

The character side grew too: major 5 is `/dev/net`, and major 1's
MultIO console is now one of two consoles, the MultIO ACE or the SS1's
2651, chosen at boot.  1.6 had the MultIO only.

The mechanism is as important as the list.  In 1.6 every driver was
linked into the kernel and named in the static `biosw`/`ciosw` tables;
a new controller meant a new kernel.  In 2.x every driver is a 4K
module linked at 0xd000 against the kernel's symbol table, appended to
the image, and discovered at boot: the module's header (`struct
ovlhdr`) declares its major and entry points, an `init()` says whether
its card is present, and `intr_attach()` finds its interrupt line by
tickling the card and reading the 8259's request register.  A driver
whose card is not in the machine is never placed and its major answers
as `nodev`; adding a controller does not change the kernel's text or
its link.

The nine modules do not all want a watchdog, but the ones that do share
one clock (`ovltick()`), and their init-only halves are folded into
`highmem.o` so the 4K page holds only what the running driver needs.

## Other functional extensions

These came with the same work and are worth knowing:

- **Disk labels and slices.**  1.6 hard-wired its swap geometry
  (`swapsize 2048`, `swapaddr 18448` in `con.c`) and the drivers read
  geometry from the minor.  2.x reads an on-disk label (`sys/dlabel.c`,
  `include/sys/dlabel.h`): eight cylinder-granular slices a-h, with
  `c` the whole drive, and the boot as a drive attribute rather than a
  slice.  The kernel, mkfs and every reader take the geometry from it;
  the formatters write it from their own drive tables.  Swap is a
  slice and its map is sized from the label at boot.
- **Runtime board discovery.**  One kernel boots the SS1, the
  Wunderbus and the MultIO: the PICs are probed, the console and clock
  entry points are chosen from what answered, and each card's
  interrupt line is discovered rather than compiled in.  1.6's
  interrupt table named its handlers.
- **`/dev/net`.**  The network interface is a character device
  (`sys/net.c`) with framed read/write through the ESP32 card; it did
  not exist in 1.6.  See `sys/NET.md`.
- **`/dev/mem` and `/dev/ports`** keep their 1.6 meanings; both are
  now served by `sys/memdev.c`'s `memread`/`memwrite`, which tell
  memory from ports by the major.

## Where to check

- 1.6 syscall table and buffer arithmetic:
  `kernels/micronix.161.sym` and the `_syssw`/`_nbuf` symbols in the
  image; the 1.6 source in `extra/ORIGINAL/BU12/lens.m16/usr/src/`
  (`con.c`, `uio.c`, `exec.c`, `main.c.partial`).
- 2.x: `sys/consts.c` (syscall and device tables), `sys/ovl.c` and
  `include/sys/ovl.h` (drivers), `sys/malloc.c` (tasks and segments),
  `sys/exec.c`, `sys/uio.c` (cache), `sys/dlabel.c` (labels).
- `ARCHITECTURE.md` in this directory is the map of how the pieces
  fit together; `INTR.md` is the interrupt design.
