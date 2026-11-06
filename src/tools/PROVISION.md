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
      "wiring": "wb,mio,vi0=djdma,vi1=hddma,vi3=ncr,vi4=nic,vi5=ide",
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
  verbatim, exactly as the option is typed: the machine's boards, then the
  cards' assignments.  Boards are `ss1`, `wb` (a Wunderbus I/O), `mio` (a
  MultIO), and the pairs `wb,mio` (a Wunderbus with one MultIO attached,
  polled) or `mio,mio` (two MultIOs cascaded).  The rest is
  `viN=card`, e.g. `"wb,mio,vi0=djdma,vi1=hddma,vi3=ncr,vi4=nic,vi5=ide"`;
  `irqN=` and `int=` remain available as overrides.
- **root / swap** — the kernel's setdev arguments, named as device
  tokens, not raw major/minor.  `show` resolves and prints them; `boot`
  does not pass them to anything, and no kernel is patched.
- **volumes** — the disks, each a controller + unit + geometry + image.
  A volume may also carry **files**: a list of `path`/`source` pairs
  placed in the image, each `path` absolute in the image and each
  `source` a file in the tree.  They are how a machine gets a
  particular `.login` or `/etc/rc` without editing the tree's defaults.
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
| boot | mon500: 0x08 · 0x10 · 0x20 · 0x40 · 0x80; 375/447: 0x08 · 0x10 · 0x00 | mon500: floppy / hdcdma·disk3 / ide / scsi / hdca·disk2.  375/447: hdcdma / djdma / hdca·ide |
| nomon | mon500: 0x7c when no `boot.device`; 375/447: 0x04 | boot; for mon500 the usual set plus multiuser |
| console | none for mon500 | wb / ss1 — checked; mon500 probes the Wunderbus first and the SS1 after |
| xterm | 0x0700 (3 bits) | which of uart0 / ttyB / ttyC go to xterms |
| clock | 0x0800 · 0x1000 | 1hz / noclk / normal |

mon500 no longer gives the console a switch bit.  The old switch 5 is now
the floppy-enable bit; a mon500 profile's `console` is only checked, and
the monitor's probe order is fixed.

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
read the rest off its own card.  The tool chooses it from the volume's
controller, which is the one thing the image cannot be shared across:

    hdcdma   src/micronix/stand/boot/bootimg
    hdca     src/micronix/stand/boot/hdcabootimg
    ide      src/micronix/stand/boot/idebootimg
    ncr      src/micronix/stand/boot/ncrbootimg
    djdma    per medium: djbootimg-d8ss, djbootimg-d8ds, djbootimg-d5ds,
             djbootimg-d5soft

`dist/create_dist` picks `djbootimg-d8ds` for the eight inch diskettes
and `djbootimg-d5soft` for the five inch ones by hand today.  A
`djdma` volume is a removable medium and is not a `mkfs` target, so the
tool reports it rather than choosing an image.

disk1, disk2 and disk3 have boot readers in the level 1 rom's
controller table (`ctable`, src/micronix/stand/roms/l1boot.s) and no
image to put in front of them.  The tool's major table does not know
those names, so a volume on one of them cannot appear in `root` or
`swap`, and `mkfs` reports it rather than guessing a level 2.  The ND
network disk has no row in that table at all.

### The rom decides what the boot bits mean

mon500's switch is software-defined (`mon500.s`, switch equates):

| bit | meaning |
|---|---|
| 0x04 | multiuser after the single-user idle timeout |
| 0x08 | enable boot from floppy |
| 0x10 | enable boot from hdcdma/disk3 |
| 0x20 | enable boot from ide |
| 0x40 | enable boot from scsi |
| 0x80 | enable boot from hdca/disk2 |

Any of 0x08..0x80 enters the level 1 rom; with none set the monitor
runs.  `boot.device` names one class and the tool adds 0x04; `nomon`
with no device takes the usual set 0x7c.  The level 1 rom probes the
enabled classes in policy order (floppy, mfm, ide, scsi, hdca/disk2)
and boots the first with a label, so no menu input is needed.

375/447 still read the old boot-address field, so the tool packs the
old bits only for those roms (`boot` 0x08/0x10/0x00, `nomon` 0x04).

`boot.j2` is the DJ-DMA card's J2 strap.  `ab` (the default, and how the
real machine is wired) does not run the card's own bootstrap; `bc` does,
which is what the stock monitor's `djdma` path needs.  It is a field
because it is a strap on a board being simulated and not a property of a
disk — and because `-b mon447.bin -c 0x14 -j bc` is a machine somebody
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
    mkfs <sim>         mnix mkfs <image> -i <bootimg> -f     filesystem + boot,
                       then the volume's file overrides
    files <sim>        write each volume's file overrides into its image
    boot <sim>         launch the volumes; d1p headless by default, d1 with
                       -g/--debug, logging to /tmp/hwsim-<sim>.log.  Both
                       pass -H, so a task-0 halt reaches the monitor trap
                       instead of ending the simulation.
    volumes <sim>      each volume: controller unit geometry image
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

`mkfs` writes the volume's controller level 2 into the boot area, by
the table above.  A volume whose controller has no level 2 - `djdma` is
the only one - is reported and the verb stops rather than making an
unbootable image.

`volumes` is the machine-readable form of the volume list, one line per
volume, for host scripts.  `disks/hdinstall/create_sim_images` uses it
to build the images the file names, and `make images` runs that after
building the userland, boot images and host tools.  The builder makes
hdcdma/m40 volumes, which is what the presets use; a volume on another
controller or geometry, or an image outside the tree, is reported and
skipped.

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
                               one install diskette, for one controller
    disks/hdinstall/build_dist, dist_boot
                               the worked example, done by hand today

setdev is not called: `show` resolves the `root` and `swap` tokens and
prints the major/minor, and nothing patches a kernel.  The media changer
(MEDIACHG.md) is how a boot's floppies get loaded.
