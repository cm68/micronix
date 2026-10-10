# End-to-end test: two-drive install, native compile, and timing

Checkpoint as of 2026-10-09. This is the running record for the full
bring-up: install the 9-diskette distribution onto two drives, run the
native self-host compile of the source tree, and then measure the
simulator's wall-clock time and its ratio to a real Z80.

**Paused** pending the interrupt-controller redo (see "Where we stand").
Everything here is a pointer for picking the work back up when the redo
lands.

---

## The goal, in three stages

1. **Install** — `build_2drive` makes two fresh hard-disk volumes
   (`/tmp/q540root.img`, `/tmp/q540usr.img`); three boots copy the nine
   distribution diskettes (`dist/mx2.0-8-1.IMD` .. `-9.IMD`) onto them.
2. **Compile** — a fourth boot runs the native self-host build: mount
   `/usr/src` on drive 1 and `make` the whole tree in the guest.
3. **Timing** — run that compile again on **both** simulators, **twice
   each**, and report wall-clock time and the cycle ratio to a real Z80.

---

## Tooling (all in this directory)

| file | what it does |
|------|--------------|
| `build_2drive [dir]` | builds `q540root.img` + `q540usr.img` (default `/tmp`) |
| `install5.login` | `/.login` for install passes 1 and 2 (4 floppies, `cptree`) |
| `install9.login` | `/.login` for pass 3 (1 floppy) |
| `compile.login`  | `/.login` for the compile pass (`mount` + `make`) |
| `boot_vol`       | boot harness for a `.vol` (m16) image; `-c 0x0c` + pty |
| `create_vol`     | build a single bootable m16 `.vol` |

`build_2drive` writes `install5.login` as `/.login`; the pass markers are
`=== PASS-DONE` (install) and `=== COMPILE-DONE` (compile, names a make
that *returned*, not one that succeeded — check the log for the last
`Build successful:` and any error lines).

### The d1 boot command (from `README` in this directory)

Each pass is the same command, only the `djdma0..3` floppies change. The
compile pass drops the floppies and uses `compile.login` as `/.login`:

```
cd src/hwsim/d1
./d1 -H -b ../../micronix/stand/roms/mon447.bin -B hdcdma \
    -S ../../micronix/sys/unix -L /tmp/boot.log \
    hdcdma0:/tmp/q540root.img hdcdma1:/tmp/q540usr.img \
    djdma0:../../dist/mx2.0-8-1.IMD djdma1:../../dist/mx2.0-8-2.IMD \
    djdma2:../../dist/mx2.0-8-3.IMD djdma3:../../dist/mx2.0-8-4.IMD
```

Passes: 1 = disks 1-4, 2 = disks 5-8, 3 = disk 9 (write `install9.login`
over `/.login` first). Compile = write `compile.login` over `/.login`,
then the same command with no `djdma0..3`.

### Headless boot gotcha

`d1` wants a controlling terminal for uart0 (the console). Run in the
background with no tty it dies with `uart 0 wants the controlling
terminal`. Wrap it in a pty:

```
cd src/hwsim/d1
rm -f /tmp/boot.log
script -qec "./d1 -H -b ../../micronix/stand/roms/mon447.bin -B hdcdma \
    -S ../../micronix/sys/unix -L /tmp/boot.log \
    hdcdma0:/tmp/q540root.img hdcdma1:/tmp/q540usr.img" /dev/null
```

(There is no `-c` bit for "uart0 on a pty"; `-c 0x100`/`-c 0x700` put
the uarts in xterms, which need a display. `boot_vol` uses `-c 0x70c`.)

### The two simulators

- `d1`  — normal (DEBUG) build: `make` in `src/hwsim/d1`.
- `d1p` — fast (-DNODEBUG) build: `make NODEBUG=1`. Linked against
  `libsim-nodbg.a`; objects kept apart as `*-nodbg.o`. `d1` runs it via
  `-F` (the performance simulator).

The sim prints the timing line at exit:
`sim: N cycles in T s = M cyc/s (R x a 4 MHz Z80)` — `R` is the ratio the
timing ask wants, total cycles counted.

---

## Results so far

### 1. The lock "in use by pid" kill — FIXED (committed `d2107e04`)

The compile died with `in use by pid` lock messages. Root cause: the
symbol loader `load_task_syms` re-opened the root image read-only with
`IMD_INPLACE` (which takes the advisory flock) on **every exec**, so each
new process grabbed the lock and leaked the fd. Fixed by adding
`IMD_INPLACE_RO` (read-only, no lock) and using it for read-only opens,
plus closing the fd on `imd_load`'s error path. Verified: zero "in use"
messages.

### 2. The lseek change — REVERTED (was a misdiagnosis)

While chasing the ar bug I changed `seek` to return the position in
DE:HL via a `u.offset` store, on the theory that the firmware's task
restore does not put HL' back from `u.hl2`. **That theory is wrong** —
`gotask` in `mon447.s` pops the shadow bank, so `u.hl2` does reach HL'.
The correct contract (from the user, and the `kernel-de-return-vs-whitesmiths-frame`
memory note):

- the original Micronix `seek` returns **nothing but the carry flag**
  (error);
- new kernels return the position in **HL':HL** (high half `u.hl2`, low
  `u.hl`);
- `lseek` is syntax sugar that passes a long;
- returning `u.de` breaks old Whitesmiths binaries (their `c.ret`
  restores SP from DE).

`sys/sys2.c`, `sys/reg.c`, `lib/libunix/seekraw.s` were reverted to HEAD
and `sys/unix` rebuilt on the HL':HL contract. lseek and seek work; there
is nothing to do here.

### 3. The ar bug — UNRESOLVED (blocked)

`ar: libunix.a not in archive format` — object data lands at offset 0,
over the 2-byte archive magic (`ARMAG` = 0xff65), so the next `ar`/`ld`
rejects it. This is **not** lseek (lseek/seek work). The whole `ar`
write path was read — `rcmd`→`init`/`movefil`/`install`/`ranlib`, plus
`putmag`/`getmag`, `puthdr`/`gethdr`, `putfield`/`getfield`, `copyfil` —
and is correct on inspection. So it is a runtime `long` (HL':HL) failure
on the Z80, most plausibly in:

- `movefil`'s size reconstruction — `((long)stbuf.st_size0 << 16) + stbuf.st_size1`
  (`cmd/ar/ar.c:1074`), or
- `ranlib`'s offset math — `idxsz`, `base`, `moff[]`
  (`cmd/ar/ar.c:826,846,851-853`).

A wrong size/offset makes a member write land on the magic. Reproducing
it needs a guest run (the host `mxar` takes the `#ifdef linux` branch and
can't hit the Z80-only path), which is blocked by #4. The next step is
either a guest run of `ar` on a couple of `.o`s and a hexdump of the
archive, or a look at ccc's `long` codegen for those three expressions.

### 4. Boot hang at the signon — the interrupt-controller redo

The guest reaches the single-user signon, then spins at ~99% CPU before
`/.login` runs. This is the user's parallel interrupt-controller rework
(uncommitted: `src/hwsim/d1/{disk3,djdma,hdca,hddma,ide,multio,ncr5380}.c`
plus `sys/low.s`, `sys/multio.c`, `include/sys/ovl.h`). It blocks every
guest run — install, compile, ar reproduction, timing.

---

## Where we stand

| item | state |
|------|-------|
| install/compile tooling (`build_2drive`, login scripts) | committed |
| lock kill (`IMD_INPLACE_RO`) | committed `d2107e04` |
| lseek fix | **reverted** — correct as-is (HL':HL) |
| ar bug | unresolved — runtime `long` issue, blocked |
| boot (install/compile/timing) | blocked on interrupt redo |
| timing run | not started — blocked on ar bug + boot |

---

## The timing ask (verbatim)

> "do it again, twice, with each simulator (normal and fast) and tell me
> the wall clock time and ratio to a real z80 counting total cycles"

How to run it once the redo boots and the compile succeeds:

1. `./build_2drive` (fresh volumes) then the three install passes, then
   `compile.login`.
2. For each of `d1` and `d1p`: run the compile boot **twice**, recording
   for each run the wall-clock time and the sim's exit line
   `sim: N cycles in T s = M cyc/s (R x a 4 MHz Z80)`.
3. Report the four wall-clock times and the four `R` ratios (total cycles
   counted, i.e. the sim's cycles/sec against a real 4 MHz Z80's
   4,000,000 cycles/sec).

Build the fast sim first: `make NODEBUG=1` in `src/hwsim/d1` (leave `d1`
in place — both link).

---

## How to resume

1. Wait for the interrupt-controller redo to land and the guest to boot
   past the signon (`/.login` runs).
2. `make` the kernel (`sys/unix`) and the userland so the volume and the
   guest binaries match — `sys/unix` is already on the HL':HL contract;
   `create_vol`/`build_2drive` copies `sys/unix`, never a stale
   `kernels/micronix.ccc`.
3. Re-run `build_2drive` + the three install passes + `compile.login`.
4. If the compile hits the ar bug, chase it per "The ar bug" above.
5. Then do the timing ask.
