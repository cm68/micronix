# Installing Micronix

How to build this tree, get Micronix running, and put it on media — in a
simulator first, because that costs nothing, then on a hard disk or a
diskette.

There are two simulators and they are good at different things.

`src/usersim/sim` runs Micronix *user* programs against the `filesystem/`
directory, as ordinary host processes. It is the fast one: no instruction
interpretation, no boot, and you can get a shell in it in seconds. What it
cannot do is boot — there is no kernel in it, just the system call
interface faked out against the host.

`src/hwsim/d1` is a whole MPZ80 with the disk controllers, the monitors,
interrupts and memory mapping. It boots a real kernel off a real disk
image. It is the slow one and the only one that can tell you whether the
kernel works.

Everything below assumes you are at the top of the tree.

------------------------------------------------------------------------

## 0. What you need

A host C compiler and `make`. For the user-mode simulator, `bison` and
`lib32ncurses-dev` as well — it is the only part of the tree with a yacc
grammar and a 32-bit ncurses dependency:

	apt install build-essential bison lib32ncurses-dev

Nothing else. The compiler this tree builds with is in the tree.

## 1. Build it

	make

That builds three things: `bin/sim` and `bin/d1`, the two simulators; and
`filesystem/`, the 1.3/1.4 userland read off the distribution images in
`disks/dist` by `src/tools/readall`, with this tree's own `include`,
`lib`, `cmd` and `sys` copied into `filesystem/usr/src` beside it. It
finishes with `cd src ; make`, which builds the host tools and the
simulators in place.

That is enough to *run* what the distribution shipped. To build the tree
itself — the kernel, the commands, everything under `src/micronix` — you
need the cross compiler first, because nothing else can compile anything:

	cd src/micronix
	make hostcc		# the compiler: mxccc, with the host cc
	make -C lib		# libc, libu, libccc, with mxccc
	make hostrt		# the archives where mxccc looks for them
	make			# cmd, sys (the kernel), stand, cpm
	make install		# the Z80 binaries into filesystem/

The order is the whole of it and cannot be reordered: `hostcc` needs the
host cc, `lib` needs `hostcc`, `hostrt` puts `lib`'s archives where
`mxccc` looks for them at link time, and everything else needs all three.
`make bootstrap` is those four lines as one target.

`make install` writes into `filesystem/` — specifically into
`$(ROOT)`, which defaults to `~/src/micronix/filesystem`. If your tree is
somewhere else, say so:

	make install ROOT=/path/to/tree/filesystem

The kernel comes out as `src/micronix/sys/unix`. It is not linked by the
rule above like other programs: `sys` is three overlays at a fixed base,
so the link produces `unix.link` and the driver modules are appended to
make the bootable image. `unix` is the one you want and the one every
script here copies.

## 2. Run it the quick way

	make test

which builds the userland simulator and starts it against `filesystem/`.
You get a Micronix shell. Inside it you can, for instance, rebuild the
kernel natively:

	cd /usr/src/sys
	make

The shell in there is the 1982 one and it has no compound commands — no
`&&`, no `||`, no `if`. Use `;` and separate lines.

## 3. Run it on a hard disk

Two scripts in `disks/hdinstall` make a bootable volume from the tree and
boot it. Neither needs a blessed image or a snapshot: `mnix initialize`
simulates what FORMATMW does, `mkfs` makes the filesystem, and the
userland is copied on.

	disks/hdinstall/create_vol myvol	# writes disks/hdinstall/myvol.vol
	disks/hdinstall/boot_vol myvol

`create_vol` wants three things to exist first: `stand/boot/bootimg-m16`,
a populated `filesystem/`, and `src/micronix/sys/unix`. If it stops, it
says which one is missing. The volume it makes is an `m16` — 306
cylinders, 6 heads, 17 sectors, 31212 blocks, about 16 meg — and its
kernel is stamped `rootdev 3/8` on the way in, so what boots is the
tree's current kernel and not a copy from `kernels/`. The `8` is the
`m16`'s slot in the drive table; those bits are what the driver reads
for the timing the label does not carry.

`boot_vol` runs `d1` with `-c 0x70c`: boot the HD-DMA (0x08), skip the
monitor (0x04), and put all three uarts in xterms of their own (0x700) so
the console does not wedge the window you started it from. If you would
rather have the console where you are:

	disks/hdinstall/boot_vol -c 0x0c myvol

The NCR 5380 SCSI card boots the same way, with its own pair of scripts:

	disks/hdinstall/ncr_create_vol myvol
	disks/hdinstall/ncr_boot_vol myvol

The volume is the same layout — a target on the 5380 reports its own
geometry and a volume laid out for the HD-DMA reads back block for block
through it — so only two things differ: the boot in block 0 is `ncrboot`
(`stand/boot/ncrbootimg-m16`), and the kernel is stamped `rootdev 5/0` —
major 5 is the 5380's slot in `biosw[]`, minor 0 is target 0 on the bus.
`ncr_boot_vol` uses `mon500`, the only ROM that has `ncrboot`, and
`scsi0:` for target 0, with `-c 0x71c` (0x18 boot the 5380). There will be
other SCSI controllers; this is the one whose driver is `sys/ncr.c`.

The prompt is `#`. Wait for it. A welcome *without* a prompt is a wedge,
not a quiet shell — see §7.

## 4. Make the install diskettes

	make bootdisks

at the top of the tree builds three, into `disks/bootdisks`, from the
tree and nothing else:

	d8ss.img	8 inch, single sided	77 cyl, 1 head,  15 x 512	1125 blocks
	d8ds.img	8 inch, double sided	77 cyl, 2 heads, 15 x 512	2250 blocks
	d5ds.img	5 1/4, double sided	40 cyl, 2 heads, 10 x 512	 760 blocks

They are build output, gitignored, regenerated the way a `.vol` is. Each
is a whole small Micronix whose root *is* the diskette: the loader in the
sectors in front of the filesystem, the filesystem behind it, and one
`struct dlabel` at byte 256 of device block 0 describing both — so the
diskette says what it is rather than being whatever the name it is
reached by says. `disks/bootdisks/README` has the contents and the
arithmetic; there is room for exactly that list and nothing more.

The 5 1/4 single-sided medium is deliberately not built. 194 K cannot
hold a running system and a diskette that can only be a data carrier is
not worth a media row.

## 5. Install from a diskette

This is the same install the hardware did, and what the diskettes exist
for: boot a floppy whose root is the floppy, and have it format and
populate a hard disk. Make a blank target volume and boot the diskette
with the target attached:

	src/tools/mnix initialize m16 /tmp/target.vol
	cd src/hwsim/d1
	./d1 -b roms/mon447.bin -B djdma -5 ../../disks/bootdisks/d5ds.img \
		-H hdcdma0:/tmp/target.vol

An eight-inch diskette goes on the eight-inch port as a bare filename;
only the 5 1/4 needs `-5`. `-H` is "do not exit when task 0 halts" and is
worth passing always.

When the welcome finishes and the `#` appears, type

	source m16init

which mkfs's `/dev/m16a`, fsck's it, mounts it on `/b`, copies the
running system across with `cptree`, relinks `/dev/root`, points the
copy's kernel at the drive with `setdev`, and unmounts. It prints

	Disk initialization complete.

when it is done and not before; `cptree` is copying a filesystem through
a simulated Z80, so give it minutes on a 16 meg volume. Then boot the
drive:

	./d1 -b roms/mon447.bin -B hdcdma -H hdcdma0:/tmp/target.vol

`source m16init` rather than `m16init`, because the shell's path is
`/bin` and `/usr/bin` and the script is in the root directory. `source`
is a builtin and reads the file where you name it.

## 6. Build the system inside Micronix

The point of `filesystem/usr/src` is that the system can rebuild itself.
Boot anything that gives you a shell — the user-mode simulator, a volume,
a diskette — and:

	cd /usr/src
	make

which recurses over `lib`, `libexec`, `cmd` and `sys` and rebuilds them
with the compiler that is *inside* Micronix rather than the one on the
host. The first build of the kernel there takes a while and wants the
volume to itself.

There is a wrinkle worth knowing before you chase it: the clock. The
machine has no year in its RTC — the year comes out of the filesystem's
superblock `s_time` — so a volume that was never seeded reports 1970, and
`make` then believes every file it just built is in the future and
rebuilds it forever. `mkfs` seeds `s_time` now; a volume made by an older
one will not have it.

## 7. When it does not come up

**A welcome with no prompt is a wedge, not a quiet shell.** They look
identical if you are waiting for something that is never coming. What it
looks like from outside: system calls stop while clock interrupts keep
arriving, and both simulators can tell you which. `d1 -L <file>` tees the
console, and `-t all` turns on every trace bit — all of them, because
`-t syscall` and friends each show one layer and the interesting thing is
usually where two layers disagree. Send the trace somewhere by name with
`-D <file>`: the default is a file called `logfile` dropped in whatever
directory you are standing in, which is how one reached 62 MB inside the
tree. Logs and traces go somewhere like `/tmp`.

**Give `d1` its flags before its drives.** `d1`'s option loop stops at
the first argument that is not an option, and everything after it is
taken as a drive. A `-H` that ends up after `hdcdma0:vol` is not an
option at all — it is a floppy *file* named `-H` — and `d1` exits before
running an instruction, with `sim: 0 cycles` and no diagnostic naming the
argument. It reads exactly like "this image will not boot".

**Writes go to a delta.** A run writes to `<image>-delta` beside the
image and leaves the image alone. The next run applies it again, so a
stale delta is a stale disk. `boot_vol` removes it for you; if you are
running `d1` by hand, remove it yourself. The host checkers apply the
delta too, so a guest-written disk can be checked on the host with no
merge step.

**The guest runs the volume's kernel, not the tree's.** A volume booted
by `boot_vol` runs `/micronix` *on that volume*, and a diskette runs its
own. `-S` points the tracer at a symbol file; it does not change what
runs. After a kernel change, rebuild the volume (`create_vol` again) or
the diskette (`make bootdisks`) — never glue a new kernel into a volume
that has been booted.

**Do not point a floppy's swap at a drive.** A floppy-only system is
configured `swap dev: nodev/0` and that is correct, not a missing
setting: `fork` only swaps a child out if there *is* a swap device, so
with `nodev` the system runs entirely in core. Pointing a floppy's swap
at a drive turns a system that does not swap into one that does, and then
it spends its time in the scheduler instead of in your program. This is
about the diskette. The volume `m16init` builds is a different case and
gives `/b/dev/swap` the same `m16` node `/dev/root` gets, which is what a
drive with room wants.

**Driving `d1` from a script needs a pty.** The simulator wants a
controlling terminal, so an unattended run is `script(1)` or a `forkpty`
with an expect loop around it — `src/micronix/cmd/sh/regress.sh` is one in
the tree. Two things bite. Match only on text that can appear in a
command's *output*: the shell echoes what was typed and the prompt is
already on the screen, so a pattern naming the command itself matches
before the command has run. And leave the timeout room — `cptree` through
a simulated Z80 is minutes and `fsck`'s bad-block hunt longer, so a short
per-expect timeout reads as the simulator exiting on its own partway
through.

## 8. Where to read further

	BOOT.md			every way the machine gets from reset to a
				running kernel, and where the code lives
	DJBOOT.md		the DJ/DMA floppy path in depth
	disks/bootdisks/README	what is on an install diskette, and why
	src/micronix/sys/DISKLABEL.md	the disk label, both idioms
	src/micronix/stand/boot/README	the boot blocks
	disks/hdinstall/README	the authentic install, on a simulated drive
	disks/hdinstall/REGEN	rebuilding it from source, in stages, and
				which half of that goal is met
	ATTRIBUTIONS.md		where the recovered pieces came from
