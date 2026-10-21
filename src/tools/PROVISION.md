# Provisioning tool

A command-line tool for provisioning the hardware simulator (hwsim/d1).
It manages `sim.json` — the simulator definitions — and drives the
lifecycle of a sim's block devices.  It replaces the per-card scripts
(`create_vol`, `boot_vol`, `build_dist`) with one data-driven tool: the
differences between cards become fields in a file instead of near-copies
of a shell script.

This is the design; the tool is not built yet.

## The abstraction

A **sim** is the addressable unit, and a sim is a collection of
**volumes**.  A volume is a disk on a controller and nothing more — it
names its geometry (for formatting) and its controller (for the boot
block and the wiring), not a role.  Root and swap are attributes of the
*kernel* (setdev's two arguments), not of any volume.

## sim.json

```json
{
  "sims": {
    "q540-dual": {
      "emulator": "hwsim/d1",
      "kernel":   "src/micronix/sys/unix",
      "boot": {
        "monitor": "micronix/stand/roms/mon500.bin",
        "level1":  "micronix/stand/roms/l1boot.bin",
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
        { "controller": "hdcdma", "unit": 0, "geometry": "q540",
          "image": "disks/q540-dual/root.img" },
        { "controller": "hdcdma", "unit": 1, "geometry": "q540",
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

- **boot** — the boot chain: which rom is level 0, which rom is level 1,
  and which controller the machine boots from (below).  Level 2 is the
  volume's, because it is the medium's.
- **config** — the `-c` configuration switch, broken into named fields
  (below).  The tool composes one packed `-c` number from them.
- **wiring** — the `-w` interrupt routing: a profile name, then optional
  `key=value` overrides (`"ss1,irq14=uart_rx"`).  It is passed through to
  the simulator verbatim, exactly as the `-w` option is typed, so the
  interrupt topology is persisted beside the device spec and not
  re-derived from the volume list.
- **root / swap** — setdev's two arguments, named as device tokens, not
  raw major/minor.
- **volumes** — the disks, each a controller + unit + geometry + image.
- **mediasets** — the removable-media sets (MEDIACHG.md), turned into
  the `-e` manifest by `dist/distsets`.

## Device naming

A device is `controller:unit<slice>`, no spaces: `hdcdma:0a`,
`hdcdma:0c`, `ncr:0d`.  The major numbers are fixed, so the JSON never
stores one:

    hdca = 1   djdma = 2   hdcdma = 3   ide = 4   ncr = 5

The minor encodes the slice in the top three bits and the unit below
(`devslice`, sys/include/sys/con.h; the slice table is sys/dlabel.h):

    minor = (slice << 5) | unit       a=0 b=1 c=2 d=3 e=4 f=5 g=6 h=7

`c` is the whole-disk slice (`DL_WHOLE`), so `hdcdma:0a` is 3/0,
`hdcdma:0b` is 3/32, `hdcdma:0c` is 3/64.  The tool derives the numbers
it hands setdev from the token: the major table plus the `slice<<5|unit`
encoding turn `controller:unit<slice>` into major/minor.

Floppies (`djdma`) do not follow this encoding — their minors are 8-15
and the letter is the drive — and they are dist media, referenced and
loaded through the media changer, not provisioned.

## config

`-c` is one number that packs the independent things the switch register
carries.  The named fields and their bits (hwsim/hwsim.c, mpz80.c,
multio.c):

| field | bits | values |
|---|---|---|
| boot | 0x08 · 0x10 · 0x00 | hdcdma / djdma / hdca·ide — mon375/447 only, from `boot.device` |
| nomon | 0x04 | skip the monitor — mon375/447 only; mon500 reads it as "boot" |
| console | 0x08 | wb / ss1 — mon500 only; which console the monitor probes first |
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
not by who reads it (`micronix/stand/boot/README`).  Three of the four are
properties of the *sim* and the fourth is a property of each **volume**,
so that is how the JSON carries them:

| level | what it is | where it is in the file |
|---|---|---|
| 0 | the monitor rom in its socket | `boot.monitor` |
| 1 | the 4k rom in the memory board's f0 page | `boot.level1` |
| 2 | sector 0 of the boot medium, per controller | the volume's `level2` |
| 3 | the filesystem loader | what level 2 brings in; the kernel it loads is the volume's `micronix` |

`boot.level1` is the same 4k whatever the machine is — the monitor copies
it down out of its socket and enters it, and it is what walks the
controllers and finds a medium — so it is a sim property and not a
volume's.

Level 2 is not: it is the sector that knows how to read the rest off its
own card, exactly as level 3's io driver does, so it belongs to the
controller the volume is on.  A volume may name it; when it does not, the
tool derives it from the controller:

    hdcdma   src/micronix/stand/boot/bootimg
    hdca     src/micronix/stand/boot/hdcabootimg
    ide      src/micronix/stand/boot/idebootimg
    ncr      src/micronix/stand/boot/ncrbootimg
    djdma    per medium, from the medium's geometry: djbootimg-d8ss,
             djbootimg-d8ds, djbootimg-d5ds, djbootimg-d5soft

A removable medium has no volume entry to hang a `level2` on, so the
medium table carries one the same way.  `dist/create_dist` picks
`djbootimg-d8ds` for the eight inch diskettes and `djbootimg-d5soft` for
the five inch ones today, and that choice is what the tool should read
rather than repeat.

Four controllers have a level 2 and four do not: disk1, disk2, disk3 and
nd have boot readers in the level 1 rom and no image to put in front of
them, so `mkfs` on such a volume has to refuse rather than write a boot
area nothing can read.

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

### A controller with no rom-side config: nd

The network disk's card does its own network, so the rom's row for it
says only "fetch block 0" and the addressing lives in the card.  There is
nowhere on a 4k rom to keep an address, a socket or a password, so the
card's configuration is stored in its own NVRAM and the boot operation
loads it from there (`docs/hw/esp32-nic.md`).  Nothing in the JSON
carries it, and nothing should: it is not the sim's or the volume's.

## The verbs

`tool <verb> <simname> [args]`:

    format      mnix initialize <geometry> <image>     geometry label
    partition   mnix label <image> -s ...              slice table
    mkfs        mnix mkfs <image> -i <level2> -f       filesystem + boot
    boot        launch d1/d1p with the sim's boot chain + volumes
    add / del   put a volume into / take one out of a sim
    list / show sim.json

`mkfs` takes the boot image from the volume's controller — `level2` if the
volume says so, the table above if it does not — which is the one place
`create_vol`'s `bootimg="$root/src/micronix/stand/boot/bootimg"` and its
three siblings differ from each other.

`boot` generates the device wiring from the volume list — every volume's
`controller<unit>:<image>` in order — composes `-b`/`-M`/`-j` from
`boot`, the `-c` value from the config's fields, and `-e` from the
mediasets.  That generation is what turns the four near-identical
per-card scripts into one tool, and the boot chain is what makes it
possible for a script to be about a machine rather than about a card.

## Where the pieces already are

The tool is a front end over things the tree already has:

    mnix                     initialize / label / mkfs / tar / write / mknod
    setdev                   patch the kernel's root + swap (from the tokens)
    dist/distsets            print the -e manifest from dist images
    stand/roms               the level 0 and level 1 roms (make -C stand/roms)
    stand/boot               the level 2 and level 3 images (make bootimgs,
                             make djbootimgs)
    bootdisks/create_bootdisk  one install diskette, for one target drive
    build_dist / dist_boot   the worked example, done by hand today

`build_dist` and `dist_boot` are exactly what `provision` and `boot`
will do once the JSON drives them; the media changer (MEDIACHG.md) is
how the boot's floppies get loaded.
