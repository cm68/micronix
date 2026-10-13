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
      "monitor":  "micronix/stand/roms/mon447.bin",
      "kernel":   "src/micronix/sys/unix",
      "config": {
        "boot":  "hdcdma",
        "nomon": true,
        "xterm": ["uart0", "ttyB", "ttyC"],
        "clock": "normal"
      },
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

- **config** — the `-c` configuration switch, broken into named fields
  (below).  The tool composes one packed `-c` number from them.
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

`-c` is one number that packs four independent things.  The named fields
and their bits (hwsim/hwsim.c, mpz80.c, multio.c):

| field | bits | values |
|---|---|---|
| boot | 0x08 · 0x10 · 0x00 | hdcdma / djdma / hdca·ide (rom-dependent) |
| nomon | 0x04 | skip the monitor |
| xterm | 0x0700 (3 bits) | which of uart0 / ttyB / ttyC go to xterms |
| clock | 0x0800 · 0x1000 | 1hz / noclk / normal |

`boot_vol`'s `-c 0x70c` is `xterm: all` (0x700) + `boot: hdcdma` (0x08)
+ `nomon` (0x04).  The tool emits one packed `-c` — not `-B`, which
covers only the boot bits and a later `-c` would override.

## The verbs

`tool <verb> <simname> [args]`:

    format      mnix initialize <geometry> <image>     geometry label
    partition   mnix label <image> -s ...              slice table
    mkfs        mnix mkfs <image> -i <bootimg> -f      filesystem + boot
    boot        launch d1/d1p with the sim's config + volumes
    add / del   put a volume into / take one out of a sim
    list / show sim.json

`boot` generates the device wiring from the volume list — every volume's
`controller<unit>:<image>` in order — and the config's fields become the
`-c` value.  That generation is what turns the four near-identical
per-card scripts into one tool.

## Where the pieces already are

The tool is a front end over things the tree already has:

    mnix                     initialize / label / mkfs / tar / write / mknod
    setdev                   patch the kernel's root + swap (from the tokens)
    dist/distsets            print the -e manifest from dist images
    build_dist / dist_boot   the worked example, done by hand today

`build_dist` and `dist_boot` are exactly what `provision` and `boot`
will do once the JSON drives them; the media changer (MEDIACHG.md) is
how the boot's floppies get loaded.
