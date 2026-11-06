# TESTING — reports, regression matrix, and the push gate

Three things measure this tree, and they answer different questions:

| question | tool | cost |
|---|---|---|
| does the kernel still fit its memory map? | `src/micronix/sys/membudget.sh` | seconds |
| can the compiler still compile itself on the machine? | `make -C src/micronix footprint` | tens of minutes and up |
| does the system still install and rebuild on every disk card? | `./regress` | about three hours |

Nothing in the tree enforces them yet: there is no CI configuration,
no `.githooks` directory, and no `core.hooksPath`. The gate is the
checklist at the end of this file, run by hand before a push.

Build the tree once first (`make`); every report below reads built
artifacts and does not build them for you. `make clobber` is the only
safe starting point after a compiler or ABI change — `make` decides
from timestamps, and a new compiler does not touch the objects.

## Kernel memory budget — the code budget report

```
make -C src/micronix/sys unix       # the image the report reads
src/micronix/sys/membudget.sh       # default: src/micronix/sys/unix
src/micronix/sys/membudget.sh path/to/unix
```

The script is read-only. It reads the Whitesmiths object header and
symbol table of the image and of the folded objects the final link
places (`low.o`, `highmem.o`, `win.o`, `upage.o`), plus each driver
module's `.mod`, so it needs `bin/mxnm` and `bin/mxccc` (built by
`make`) and a built `sys/unix`. It compiles one expression with
`mxccc` to get `sizeof(struct buf)` and cross-checks that against what
`textpad.s` reserved for the eight boot headers; a disagreement means
the pool does not start at `_ebss` and is reported as a warning.

The current output on this tree (the module rows are compressed to two
columns here; the script prints one per line, sorted by free space):

```
kernel memory budget: /home/curt/micronix/src/micronix/sys/unix

resident    text   0x1000-0x91ae   33198
            data   0x91ae-0xada7    7161
            bss    0xada7-0xb2d4    1325
            extent 0x1000-0xb2d4   41684  SCRWIN 0xc000, 3372 free

init-only   highmem.o 0xb2d4-0xca25    5969  OVLBASE 0xd000, 1499 free
            link needs _ebss <= 0xb8af: 1499 bytes of resident growth

pool        19 bytes/header: 8 boot + 177 = 185 buffers (MAXBUFS 256)
            headers 0xb2d4-0xbff7, 24 segments = 96K of buffer data, 9 bytes spare
            every 19 bytes of resident growth costs one buffer

pages       driver slot 0xd000  4096/4096
            window      0xe000  3566/4096   530 free
            u page      0xf000  4033/4096    63 free

modules     one 4096-byte page at 0xd000
            net 4015/81 free      pty 3986/110 free
            hdca 3851/245 free    disk1 3619/477 free
            ncr 3605/491 free     tty 3597/499 free
            ide 3506/590 free     dj 3367/729 free
            mw 2896/1200 free     disk3 2319/1777 free
            disk2 2268/1828 free  nd 975/3121 free
            io 546/3550 free      mem 546/3550 free
            14 pages, 57344 bytes stacked after the link
```

Read the three hard ceilings, not the row sizes:

- **resident extent vs `SCRWIN`** (`3372 free`) — data/bss growth past
  `0xc000` collides with the screen window.
- **`highmem.o` vs `OVLBASE`** (`1499 free`) — init-only code is parked
  at `_ebss` and may run to the driver slot. This is the number most
  kernel changes spend; once it is gone the link fails.
- **the page budgets** — the driver slot is always full by design; the
  buffer window has `530 free` and the **u page has only 63**, so leaf
  code added there is effectively out of room.

The pool line is the consequence of the first: every `19` bytes of
resident growth costs one buffer, and the pool tops out at `MAXBUFS`.

## Compiler self-host footprint

```
make                                # at least src/usersim/sim and the host passes
make -C src/micronix footprint
```

or, from `src/micronix`:

```
make footprint
```

`footprint` depends on `hostpasses`, then runs
`src/micronix/libexec/footprint.sh`. The script clobbers and rebuilds
all four passes (`pass0`, `c0`, `c1`, `peep`) in both host and native
form, stages the compiler's own sources into a scratch tree, and runs
every pass over every source under the user-mode simulator
(`src/usersim/sim`). For each source it records the brk/gap and
compares the simulated output byte-for-byte with the host chain's
(`pass0` is compared with matching relative paths — the source path
spelling lands in the line markers).

It fails on any of:

- a pass running **out of memory** on a source it used to fit,
- a gap below the floor (`FLOOR`, default `256`) — still fitting, but
  one bug fix away from not,
- simulated output **diverging** from the host's.

Knobs, from the environment:

| variable | meaning | default |
|---|---|---|
| `FLOOR` | minimum acceptable gap, bytes | `256` |
| `FPJOBS` | sources measured in parallel | `2 × nproc` |
| `FPWORK` | scratch work tree | `src/micronix/libexec/fpwork` |
| `KEEP` | keep scratch files when set | unset (removed) |
| `COMPILER_ROOT` | compiler tree under test | one level up from `libexec/` |

The measured set is every `.c` in `pass0`, `c0` and `c1` except
`test.c`. The table is printed in source order, one row per source:

```
source             pass0 gap     c0 gap        c1 gap        peep gap
pass0/alloc        ...           ...           ...           ...
...
footprint: every pass fits every source, no gap under 256
```

Exit status is 0 only for that last line. `footprint: FAILED - the
self-host is at or past the tipping point` means one of the three
failure modes above; the rows and the `OUT OF MEMORY`/`DIVERGES`
messages above it name the source and the pass.

The run is long because the rows are simulated, not host processes:
c1 alone is about forty seconds per source on this host. The scratch
tree is removed unless `KEEP` is set; a failure is reproducible by
re-running with `KEEP=1`. This is the report to run after any change
under `src/micronix/libexec`, `src/micronix/cmd/ccc`, or the compiler
headers.

## Regression: the controller matrix

`./regress` is the wrapper around `disks/hdinstall/matrix_test`:

```
./regress [-1] [-x] [-s] [-i] [-c "<controllers>"] [-d <dir>]
./regress -h                       # the matrix's own usage
```

Controllers are `hdcdma` (Morrow HD-DMA), `hdca` (Morrow
HDCA/M26), `ncr` (NCR 5380 SCSI) and `ide` (the 8255/ATA board); the
default is all four, in that order, **in parallel**. `-1` runs them
one at a time, `-x` stops at the first failure (and implies `-1`),
`-c` names a subset, `-d` names the work directory.

`-s` stops after the diskette and the blank volumes are made; `-i`
stops after the install boot and the hard-disk boot write. Either is
the quick way to check a formatter, geometry or device name without
waiting for an install, let alone four builds.

### What a run does

The matrix begins with `make clobber` at the top of the tree and a
build from source of everything it uses: both simulator builds, the
host tools, the cross toolchain, the guest userland, the boot images
and the distribution media. Nothing built earlier is reused — a
regression against an old object tests old code.

Each controller is an independent machine and gets a work directory.
Each leg is two boots, with blank volumes made for it first:

1. **install** — the DJ-DMA install diskette boots (`-B djdma`), runs
   `format` on the controller the device node names (the IDE card has
   no low-level format, so `format` writes its label), then `label`,
   `mkfs` and `fsck` on both volumes, copies
   the diskette's filesystem to the root volume, points its kernel at
   the controller, and `cptree`s the distribution diskettes onto it.
   The console ends with `=== PASS-DONE`.
2. **build** — the root volume boots on its own controller with no
   floppies, mounts `/usr/src`, and runs
   `make`, `make install`, `make clobber`, `make`, `make verify`, with
   a marker after each and `=== TEST-DONE` at the end.

Both boots end with `sync; halt`. Success needs the marker, the
`halt: 0 dirty buffers` line, and the simulator exiting on its own; the
harness does not kill it. A killed simulator never flushes the
superblock, which is also why every run makes fresh volumes and gets a
fresh work directory.

### Where the results are

`./regress` uses `/tmp/regress-<date>-<time>` unless `-d` names
another directory (the matrix itself defaults to `/tmp/matrix`;
`WORK` overrides it when called directly):

```
<dir>/run.log                  the whole console, tee'd
<dir>/summary                  the aggregate verdict
<dir>/host.log                 clobber/build of the tree
<dir>/<controller>/console.log the leg's own output
<dir>/<controller>/summary     its verdict
<dir>/<controller>/install.log boot 1
<dir>/<controller>/build.log   boot 2
<dir>/<controller>/root.img    the disk /b is on (kept on failure)
<dir>/<controller>/usr.img     /usr/src (kept on failure)
```

`run.log` ends with the summary; the wrapper prints it and exits 0
only when every requested controller passed every stage.

Failure messages and what they mean:

- `install FAILED (simulator exited before the marker)` — the boot
  exited without `=== PASS-DONE`; read `install.log`.
- `install FAILED (panic)` / `(timeout)` / `(no orderly shutdown)` —
  the corresponding line is in `install.log`.
- `install FAILED (console errors)` — the log contains one of
  `not a file system`, `No more space`, `No such device`,
  `out of memory`, `cannot execute`, `cannot open`, `no label`.
- `build FAILED (no <MARKER>)` — one of `COMPILE-DONE`,
  `INSTALL-DONE`, `CLOBBER-DONE`, `GEN2-DONE`, `VERIFY-DONE`,
  `TEST-DONE` never appeared.
- `build FAILED (build errors)` — the log contains
  `Build unsuccessful`, `*** Error code`, `Don't know how to make`,
  ` differ:`, `panic:`, `out of memory`, `swap error` or
  `process table full`.

A passing leg deletes its images and keeps the logs; a failing leg
keeps everything. Do not reuse a work directory: the volumes are
locked per drive file and a stopped simulator leaves the superblock
unflushed.

### Wiring

Every leg names its own one-device-per-vector wiring:

```
vi1=djdma          the install diskette's root
vi0=...            the HD-DMA family member under test (hddma or hdca)
vi3=ncr  vi4=ide  vi5=nic
```

The alternative disk cards are left unwired rather than given a line.
Two devices on one vector is a configuration error: `intr_attach()`
gripes (`attach: probe N landed on line L, already held`) and panics
rather than putting a second handler on the line. If a new leg needs a
card, give it a vector or leave it off.

### Observed timings on this host

- full matrix, four legs in parallel: about **3 hours**
- an `-i` leg (clobber, rebuild, install, hardboot): a few minutes
- the SCSI install alone: about 5–6 minutes (the `cptree` is the slow
  part)

## Faster checks

Before reaching for the matrix:

```
make test                 # filesystem + usersim, then the recovered 1.4 shell
make selfcompile          # timed native self-compile inside usersim
```

`make test` runs the user-mode system against `filesystem/`. `make
selfcompile` runs, in the guest, `cd /usr/src ; make ; make install ;
make clobber ; make ; make verify` and prints
`=== selfcompile: <seconds>s, exit <rc>`; it is the build stage of the
matrix without a disk or a boot and is still a long simulated run.

Direct usersim use (the same invocation `make selfcompile` uses):

```
src/usersim/sim -d filesystem -r /bin/sh -c 'cd /usr/src ; make verify'
src/usersim/sim -S -d filesystem -r /bin/sh -c 'make verify'   # + accounting
```

`-d` is the root directory, `-r` runs as root, `-S` reports cycles, disk
I/O and memory at exit, and `-v <mask>`/`-t` set verbosity and tracing.

Quick matrix smoke tests:

```
./regress -1 -c hdcdma -s     # diskette + blank volumes only
./regress -1 -c hdcdma -i     # install boot + hard-disk boot write
./regress -1 -c hdcdma -i -d /tmp/smoke-hdcdma
```

## Simulator presets

The `sim.json` presets boot through the provisioning tool, not a
hand-built `d1` line:

```
src/tools/hwsim list
src/tools/hwsim show wunderbus        # the switches it would pass
src/tools/hwsim boot wunderbus        # d1p headless; log /tmp/hwsim-wunderbus.log
src/tools/hwsim -g boot wunderbus     # d1 with -g and the kernel symbols
```

The presets are `wunderbus`, `wunderbus-mio`, `mio-cascade` and `ss1`.
`hwsim` composes `-b`/`-M`/`-j`/`-c`/`-w`/`-e` from the file and does
not build disk images itself.  Build the ones `sim.json` names with:

```
make images                 # userland + boot images + host tools, then the images
make images FORCE=1         # rebuild images that already exist
sh disks/hdinstall/create_sim_images ss1   # one sim, after a build
```

It makes hdcdma/m40 volumes (a bootable root built by `create_vol`, a
blank filesystem for the other volumes) and reports volumes it cannot
build — another controller or geometry, or an image outside the tree.
`hwsim volumes <sim>` prints the list the builder walks.

## The gate before pushing

There is no automated gate in this tree — no `.github`, no
`.githooks`, no `core.hooksPath`, no server-side check. The gate is
this list, run by hand. Pick the row for what changed; when in doubt,
run the full matrix.

| changed | required before push |
|---|---|
| `src/micronix/sys`, `src/micronix/include/sys` | `membudget.sh` (headroom intact) **and** full `./regress` |
| `src/micronix/libexec`, `src/micronix/cmd/ccc`, compiler headers | `make -C src/micronix footprint` **and** full `./regress` |
| `src/hwsim`, `src/lib` | full `./regress` (every leg boots `d1`/`d1p`), plus the relevant `sim.json` presets |
| `src/micronix/cmd`, `src/micronix/lib`, `src/micronix/stand` | full `./regress` |
| `disks/hdinstall`, `sim.json`, boot media | the affected legs (`-c`, then `-i`), then full `./regress` |
| docs, man pages only | clean `make` |

Minimum for every push:

```
git status --porcelain        # empty: no untracked build output, no stray edits
make                          # builds clean from the tree
```

Cheap local reminder (still minutes, not seconds — the `-s` leg
clobbers and rebuilds the tree; the full matrix is what should stay out
of a hook):

```
make -C src/micronix/sys unix
src/micronix/sys/membudget.sh
./regress -1 -c hdcdma -s
```

`pre-push` is a reasonable place for that subset; putting the full
matrix in a hook means every push costs hours, so keep it out and run
it deliberately. Record the run directory and the summary line in
the commit or PR when the matrix is the evidence, e.g.
`Verified: ./regress -> MATRIX PASSED: hdcdma hdca ncr ide`.

## Where the pieces live

- `src/micronix/sys/membudget.sh` — the kernel memory budget.
- `src/micronix/libexec/footprint.sh` — the compiler self-host footprint;
  `make -C src/micronix footprint`.
- `regress` — the matrix wrapper; `disks/hdinstall/matrix_test` — the
  matrix itself and its `-h`.
- `disks/hdinstall/REGEN.md`, `disks/hdinstall/ENDTOENDTEST.md` — rebuilding
  the hard-disk media and the end-to-end notes.
- `src/tools/PROVISION.md` — `sim.json` and the `hwsim` provisioning verbs.
- `docs/CCC_LIMITS.md` — measured compiler syntax and resource limits.
- `src/micronix/sys/static-cleanup.md` — the resident/highmem space notes
  the budget report implements.
