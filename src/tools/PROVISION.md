# Provisioning tool

A command-line tool for provisioning the hardware simulator
(src/hwsim/d1).  It manages `sim.json` — the simulator definitions — and
drives a sim's block devices through format, partition, mkfs and boot.
The provisioning the per-card scripts (`create_vol`, `boot_vol`,
`build_dist`) do by hand becomes fields in one file instead of
near-copies of a shell script.

The tool is `src/tools/hwsim`, built by the top-level `make`.  It reads
`sim.json` from the top of the tree (or the file named with `-f`) and
implements the verbs described below; where the implementation still
differs from the design, the text says so.

## The abstraction

A **sim** is the addressable unit, and a sim is a collection of
**volumes**.  A volume is a disk on a controller and nothing more — it
names its geometry (format and the slice table) and its controller (the
boot wiring), not a role.  Root and swap are attributes of the *kernel*
(setdev's two arguments), not of any volume.

## sim.json

```json
{
  "sims": {
    "q540-dual": {
      "emulator": "src/hwsim/d1",
      "kernel":   "src/micronix/sys/unix",
      "boot": {
        "monitor": "src/micronix/stand/roms/mon500.bin",
        "level1":  "src/micronix/stand/roms/l1boot.bin",
        "device":  "hdcdma",
        "j2":      "ab"
      },
      "config": {
        "xterm": ["uart0", "ttyB", "ttyC"],
        "clock": "normal"
      },
      "wiring": "multio",
      "root": "hdcdma:0a",
      "swap": "hdcdma:0b",
      "volumes": [
        { "controller": "hdcdma", "unit": 0, "geometry": "m40",
          "image": "disks/q540-dual/root.img" },
        { "controller": "hdcdma", "unit": 1, "geometry": "m40",
          "image": "disks/q540-dual/usr.img" }
      ]
    }
  },
  "mediasets": {
    "dist8": { "volumes": [ "dist/mx2.0-8-1.IMD", "..." ] },
    "dist5": { "volumes": [ "dist/mx2.0-5-1.IMD", "..." ] }
  }
}
```

`sim.json` lives at the top of the tree, three levels up from the
binary.  `-f` names another file; if the top-level file is missing the
tool falls back to `sim.json` in the current directory.  Every path in
the file is relative to the top of the tree unless it is absolute.

- **emulator** — the directory holding `d1` and `d1p`.  The code's
  default is `hwsim/d1`, relative to the top of the tree; the directory
  in this tree is `src/hwsim/d1`, so a file should name that.
- **kernel** — the symbol file handed to `d1` as `-S` under
  `-g`/`--debug`.  A normal boot does not use it.
- **boot** — the two roms the tool passes (`monitor` as `-b`, required;
  `level1` as `-M`, optional), the boot device for the `-c` bits, and
  the DJ-DMA `j2` strap (`ab` or `bc`).  The old schema kept `monitor`
  at the top level and the boot device in `config.boot`; the tool still
  reads both.
- **config** — the `-c` configuration switch, broken into named fields
  (below).  The tool composes one packed `-c` number from them.
- **wiring** — the `-w` interrupt routing, passed to the simulator
  verbatim, exactly as the option is typed: a profile name, then
  optional `key=value` overrides (`"ss1,irq14=uart_rx"`).
- **root / swap** — the kernel's setdev arguments, named as device
  tokens, not raw major/minor.  `show` resolves and prints them; `boot`
  does not pass them to anything, and no kernel is patched.
- **volumes** — the disks, each a controller + unit + geometry + image.
- **mediasets** — the removable-media sets (MEDIACHG.md).  The tool
  writes the `-e` manifest itself, in the format `dist/distsets` prints:
  one line per set, the set name then the absolute image paths.

No field names a level-2 boot image or a per-volume kernel; those are
not in the schema the tool reads.

## Device naming

A device is `controller:unit<slice>`, no spaces: `hdcdma:0a`,
`hdcdma:0c`, `ncr:0d`.  The major numbers are fixed, so the JSON never
stores one.  The tool resolves exactly these five controller names:

    hdca = 1   djdma = 2   hdcdma = 3   ide = 4   ncr = 5

The minor encodes the slice in the top three bits and the unit below
(`devslice`, src/micronix/include/sys/con.h; the slice table is
src/micronix/include/sys/dlabel.h):

    minor = (slice << 5) | unit       a=0 b=1 c=2 d=3 e=4 f=5 g=6 h=7

`c` is the whole-disk slice (`DL_WHOLE`), so `hdcdma:0a` is 3/0,
`hdcdma:0b` is 3/32, `hdcdma:0c` is 3/64.  The tool applies that major
table and the `slice<<5|unit` formula to the token, and `show` prints
the result beside `root` and `swap`.  A controller name outside the
five, or a slice letter outside `a`-`h`, is not resolved.

Floppies (`djdma`) have no slices — their minors name the drive — and
are dist media, referenced and loaded through the media changer, not
provisioned.  The token formula is still applied to a `djdma` name, so
a `djdma` token in `root` or `swap` would be given a slice it does not
have.

## config

`-c` is one number that packs the independent things the switch register
carries.  The named fields and their bits (`pack_config` in
src/tools/hwsim.c; the bit meanings are the simulator's):

| field | bits | values |
|---|---|---|
| boot | 0x08 · 0x10 · 0x00 | hdcdma / djdma / hdca·ide — packed only when `boot.monitor` is a 375/447 |
| nomon | 0x04 | skip the monitor; mon500 reads the same bit as "boot" |
| console | 0x08 | wb / ss1 — packed when the monitor is not a 375/447; which console the monitor probes first |
| xterm | 0x0700 (3 bits) | which of uart0 / ttyB / ttyC go to xterms |
| clock | 0x0800 · 0x1000 | 1hz / noclk / normal |

The console bit and the HD-DMA boot bit are the same switch, 5: a
375/447 reads it as the boot device, so there the boot field owns it.  A
mon500 reads it as "find the Wunderbus first" (set) or "the SS1 first"
(clear).  Both boards read 0x100 as "my console in an xterm", so a
machine with both opens both windows; `console` decides which one the
monitor and level 2 use.

`boot_vol`'s `-c 0x70c` is `xterm: all` (0x700) + `boot: hdcdma` (0x08)
+ `nomon` (0x04).  The tool emits one packed `-c` — not `-B`, which
covers only the boot bits and a later `-c` would override.

## The boot chain

A boot happens in four levels, numbered by where the code comes from and
not by who reads it (`src/micronix/stand/boot/README`).  The tool
supplies the first two from the sim; the last two come with the medium:

| level | what it is | what the tool does |
|---|---|---|
| 0 | the monitor rom in its socket | passes `boot.monitor` as `-b`, and refuses a sim without one |
| 1 | the 4k rom in the memory board's f0 page | passes `boot.level1` as `-M` when the field is present |
| 2 | sector 0 of the boot medium, per controller | `mkfs` writes `src/micronix/stand/boot/bootimg` into every volume |
| 3 | the filesystem loader | what level 2 brings in |

`boot.level1` is the same 4k whatever the machine is — the monitor copies
it down out of its socket and enters it, and it is what walks the
controllers and finds a medium — so it is a sim property and not a
volume's.

Level 2 belongs to the controller: it is the sector that knows how to
read the rest off its own card.  The tool does not yet choose it by
controller; every volume gets the hdcdma image above.  These are the
per-controller images a later revision would pick from:

    hdcdma   src/micronix/stand/boot/bootimg
    hdca     src/micronix/stand/boot/hdcabootimg
    ide      src/micronix/stand/boot/idebootimg
    ncr      src/micronix/stand/boot/ncrbootimg
    djdma    per medium: djbootimg-d8ss, djbootimg-d8ds, djbootimg-d5ds,
             djbootimg-d5soft

`dist/create_dist` picks `djbootimg-d8ds` for the eight inch diskettes
and `djbootimg-d5soft` for the five inch ones by hand today.

disk1, disk2 and disk3 have boot readers in the level 1 rom's
controller table (`ctable`, src/micronix/stand/roms/l1boot.s) and no
image to put in front of them.  The tool's major table does not know
those names, so a volume on one of them cannot appear in `root` or
`swap`, and `mkfs` would still write the hdcdma image.  The ND network
disk has no row in that table at all.

### The rom decides what the boot bits mean

`boot.device` and `config.nomon` are mon447's fields: that rom reads the
boot device from the top five bits of the switch, and 0x04 on top of it
says "skip the monitor".  mon500 reads **only** 0x04 as a boot selector
— S6 is monitor or boot, and to mon500 a boot *is* the level 1 rom
(`mon500.s:443-452`) — while switch 5 picks the console.  So the tool
packs the boot bits only when
`boot.monitor` is a rom that has them; with mon500 it emits the two roms
(`-b` and `-M`) and nothing else, and the machine comes up in the level 1
menu, which is the point of level 1.

`boot.j2` is the DJ-DMA card's J2 strap.  `ab` (the default, and how the
real machine is wired) does not run the card's own bootstrap; `bc` does,
which is what the stock monitor's `djdma` path needs.  It is a field
because it is a strap on a board being simulated and not a property of a
disk — and because `-b mon447.bin -B djdma -j bc` is a machine somebody
may still want to stand up.  See `stand/boot/LEGACYBOOT.md`.

### The network disk: nd

The ND card is a block driver (major 6) and a character device
(`/dev/net`, major 5) in the kernel (`src/micronix/sys/nd.c`,
`src/micronix/sys/net.c`; see `src/micronix/sys/NET.md`).  It has no
row in the level 1 controller table (`ctable`,
src/micronix/stand/roms/l1boot.s) and no level-2 image, so it is not a
boot medium.  The tool's major table does not name it, and nothing
about the card is in `sim.json`.

## The verbs

`hwsim [-f sim.json] <verb> [sim] [args]`:

    format <sim>       mnix initialize <geometry> <image>    geometry label
    partition <sim>    mnix label <image> -s ...             slice table
    mkfs <sim>         mnix mkfs <image> -i <bootimg> -f     filesystem + boot
    boot <sim>         launch the volumes; d1p headless by default, d1 with
                       -g/--debug, logging to /tmp/hwsim-<sim>.log
    add <sim> <controller> <unit> <geometry> <image>
    del <sim> <controller>:<unit>
    list               the sims in the file
    show <sim>         one sim, then the switches it would pack

The options go before the verb: `-f` names the platform file, and
`-g` (or `--debug`) boots `d1` instead of headless `d1p`, adding
`-S <kernel>` when `kernel` is set.  `add` and `del` rewrite `sim.json` in place; `add` appends the
volume without checking that its controller or geometry is known, and
`del` needs only the `controller:unit` token.  `show` prints the sim as
JSON, then `# root`, `# swap` and the `-b`, `-M`, `-j`, `-c` and `-w`
values the boot would use.

`mkfs` uses `src/micronix/stand/boot/bootimg`, the hdcdma level 2, for
every volume.  Until the tool chooses the image by controller, a volume
on another controller needs an image built with that controller's
level 2.

`boot` first checks that every volume's image exists.  It then generates
the device wiring from the volume list — every volume's
`controller<unit>:<image>` in order — composes `-b`/`-M`/`-j` from
`boot`, the `-c` value from the config's fields, and `-e` from the
mediasets, and passes `-w` when `wiring` is present.  `root` and `swap`
are not used.

## Where the pieces already are

The tool invokes two things: `src/tools/mnix` for initialize, label and
mkfs, and the emulator directory's `d1` or `d1p` for boot.  Everything
else is input it expects the tree to have built:

    src/tools/mnix             initialize / label / mkfs (invoked)
    src/micronix/stand/roms    the level 0 and level 1 roms
                               (make -C src/micronix/stand/roms)
    src/micronix/stand/boot    the level 2 and level 3 images
                               (make bootimgs, make djbootimgs)
    dist/distsets              the -e manifest format hwsim reproduces
    disks/bootdisks/create_bootdisk
                               one install diskette, for one target drive
    disks/hdinstall/build_dist, dist_boot
                               the worked example, done by hand today

setdev is not called: `show` resolves the `root` and `swap` tokens and
prints the major/minor, and nothing patches a kernel.  The media changer
(MEDIACHG.md) is how a boot's floppies get loaded.
