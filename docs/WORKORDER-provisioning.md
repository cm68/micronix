# Work Order: the hardware simulator provisioning tool

## The job

Build the command-line provisioning tool for the hardware simulator
(`hwsim/d1`).  It manages `sim.json` — one file holding every simulator
definition — and drives the lifecycle of a sim's block devices.  It
replaces the per-card scripts (`create_vol`, `boot_vol`, `build_dist`)
with one data-driven tool, so the differences between cards become
fields in a file instead of near-copies of a shell script.

The design is already settled and written down; this order is to build
it.  Do not re-derive the design.

## Read first

- `src/tools/PROVISION.md` — the full design: the `sim.json` schema, the
  device naming, the `-c` config decomposition, the verb list.
- `README.md` — the "the provisioning tool" section, a friendly summary.
- `src/micronix/sys/MEDIACHG.md` — the media changer, which the tool's
  `mediasets` feed.

## What already exists (the tool wraps these)

- `src/tools/mnix` — the host image tool.  `initialize`, `label`, `mkfs`,
  `tar`, `write`, `mkdir`, `mknod`.  The backend for format/partition/mkfs.
- `src/tools/setdev` — patches a kernel's root + swap device numbers.
- `dist/distsets` — derives the `-e` manifest from the `dist/` images.
- `src/hwsim/d1/d1` and `d1p` — the simulator (d1p is the fast build).
- `disks/hdinstall/create_vol`, `boot_vol`, `build_dist`, `dist_boot` —
  the hand-rolled prototypes the tool subsumes.  `build_dist` + `dist_boot`
  are the worked example, done by hand today.

## What to build

A host-side command-line tool.  The tree is C-only — no C++, no Python;
host tools link with `$(CC) $(CFLAGS)`.  Live in `src/tools/`, following
the existing Makefile style.

    tool <verb> <simname> [args]

Verbs:

- `format`    — `mnix initialize <geometry> <image>` (geometry label).
- `partition` — `mnix label <image> -s ...` (slice table).
- `mkfs`      — `mnix mkfs <image> -i <bootimg> -f` (filesystem + boot).
- `boot`      — launch `d1`/`d1p` from the sim's config + volume list.
- `add`/`del` — add/remove a volume in a sim (and its backing image).
- `list`/`show` — manage `sim.json`.

Also: read and write `sim.json`, and turn its `mediasets` into the `-e`
manifest the way `dist/distsets` does today.

## Design decisions already made (from PROVISION.md)

- A **sim** is the unit; a sim is a collection of **volumes**.
- A volume is a disk on a controller (geometry + controller) — **not** a
  role.  Root and swap are the kernel's `setdev` arguments, not volume
  attributes.
- A device is `controller:unit<slice>`, no spaces (`hdcdma:0a`).  Major
  numbers are fixed (hdca=1, djdma=2, hdcdma=3, ide=4, ncr=5); the minor
  is `(slice<<5)|unit`, a=0..h=7, `c` = whole disk.  The JSON never
  stores a raw major/minor.
- `-c` decomposes into named fields — `boot` (hdcdma/djdma/hdca), `nomon`,
  `xterm` (3-bit uart mask), `clock` (1hz/noclk/normal) — and the tool
  emits **one** packed `-c` value, not `-B`.
- `boot` generates the `d1` device wiring from the volume list (each
  volume's `controller<unit>:<image>` in order).
- Floppies (`djdma`) are dist media, not provisioned; they load through
  the media changer.

## Constraints

- C-only tree, no C++.  JSON parsing in C is the one open implementation
  question: hand-roll a small parser or vendor one (lean hand-rolled).
- Volumes are `.img`/`.vol`, gitignored; never add a disk image.
- Match the existing scripts' behavior exactly — `build_dist` and
  `dist_boot` are the acceptance reference.

## Acceptance

- `tool list` / `show` / `add` / `del` operate on a `sim.json`.
- `tool format` / `partition` / `mkfs` produce the same image `create_vol`
  (or `build_dist`) does.
- `tool boot <sim>` launches `d1p` and reproduces what `dist_boot` does
  by hand: the packed `-c`, the `-e` manifest, the device wiring.
- A `mediasets` block in `sim.json` yields the same manifest
  `dist/distsets` prints.
