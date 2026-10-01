# Rebuilding the filesystem without the distribution disks

`docs/DISTRIBUTION-FREE.md`

## The current flow

`make filesystem` (the top `Makefile`) builds `filesystem/` in two layers:

1. **Base layer, from the disks.** `src/tools/readall` spews the images
   named in `disks/dist/MICRONIX` (seven IMDs) into `filesystem/`, and
   those in `disks/dist/NEWER` (three IMDs) into `filesystem/newer/`.
   This is the 1.3/1.4 userland exactly as the distribution shipped it.
2. **Overlay, from source.** `make install` under `src/micronix` then
   writes this tree's binaries, libraries, headers, man pages and `/etc`
   seeds over the top of `filesystem/`, and `make filesystem` copies
   `src/micronix/{cmd,include,sys,lib}` into `filesystem/usr/src`.

The disks are the base; the build is the overlay. Eliminating the disks
means building the base's *structure* from source and closing every gap
the overlay does not already cover.

## What the disks supply, by disposition

### Already from source — the disks are not needed for these

- `/bin` — the 104 ccc-built commands and their aliases.
- `/etc` — the seeds, installed from `src/micronix/etc`.
- `/usr/src`, `/usr/include` — the source and headers.
- `/lib` — `libc.a libu.a libccc.a libutil.a libfs.a liblc.a liby.a crt0.o`.
- `/libexec` — `pass0 c0 c1 peep astpp`.
- `/usr/man` — the tree has 284 pages across man1–5 (more than the 237
  installed); `make install-man` puts them there.
- The kernel — `sys/unix`, copied to `/micronix` by `create_vol`.
- `/dev` — the node list already lives at `src/micronix/dev/devlist`;
  `create_vol` mknod's it onto volumes. The usersim `/dev` is not yet
  generated from it (see step 4).

### Superseded — drop, do not port

- `/lib` Whitesmith archives: `libws.a libwsc.a libm.a libmd.a libmdc.a
  libp.a libS.a chdr.o uhdr.o`.
- `/include` Whitesmith headers: `8080.h cpm.h pascal.h std.h stdio.h`
  and the rest.
- `/old` — the toolchain and `ptc`, already moved there.

### No source — port, write, or drop

- `/bin` — 15 binaries; this is the `cmd/TODO` "to port" list.
- `/usr/bin` — 17 "Software Tools": `archive change common compare
  compress concat copy detab entab expand include kwic macro overstrike
  ratfor translit unrot`.
- `/usr/help/new`, `/usr/lib/lex`, `/usr/cpm/include`.

### Boot / install content

`finstall m5init m10init m16init hdinit` and `ld.o`. The rebuild of the
distributed standalone from source is the open half `disks/hdinstall/REGEN.md`
names.

### A parallel tree

`/newer` (1.6M) — the 1.4/1.41 distribution read into its own subtree and
left alone. It is reference material and needs a keep / merge / drop call.

### Junk

`core.4110` (a 1.4M core dump), `/tmp` run leftovers, `/bin/b1`.

## The work, in order

1. **Skeleton without readall** — a source list of the directories plus
   `.sh`/`.login`. `make install` already `mkdir -p`'s most of what it
   writes; the skeleton only has to name the rest.
2. **Drop the superseded** (the old `lib`/`include` entries above).
3. **Close the porting gap** — the 15 `/bin` programs and the 17
   `/usr/bin` tools. This is the long tail; several are write-fresh or
   drop (`cptree`, `newuser`, `print`, `td`), the Software Tools have
   published C sources to port from.
4. **`/dev` from `devlist`** — generate the usersim `/dev` symlinks from
   `src/micronix/dev/devlist` the way `create_vol` does for volumes. The
   old readall `/dev` also carries `root` and `swap` as fixed `bdev(2,8)`
   (floppy) aliases; the modern tree makes those per-machine in
   `install-hwsim`/`create_vol`, not in `devlist`.
5. **Resolve the boot/install content** — regenerate the install scripts,
   or accept a disk-dependent "authentic install" path that stays beside
   the source path.
6. **Decide `/newer`** — drop it, or fold its one genuinely-new thing in.
7. **Scrub junk** — `core.4110`, `b1`, `/tmp`.

## Where things live

- `src/tools/readall.c` — the disk reader that builds the base layer.
- `disks/dist/MICRONIX`, `disks/dist/NEWER` — the image manifests.
- `src/micronix/dev/devlist` — the source of `/dev` node names and majors.
- `cmd/TODO`, `cmd/TODO.md` — the `/bin`-without-source accounting.
- `disks/hdinstall/REGEN.md` — the hard-disk/standalone half of the goal.

<!-- vim: set tabstop=4 shiftwidth=4 noexpandtab: -->
