for a blast from the past, type:

 make test

 you now are running the recovered 1.4 shell and can do a lot
 including: (this builds the kernel)

	cd /usr/src/sys
	make

or, for a quite strange experience,

    src/usersim/sim bin/man sh | less

	(run the simulated z80 micronix man program on sh, and pipe it to linux less)

---------------------

three version numbers turn up in here and they do not mean the same
thing.  this tree is released as micronix 2.0, which is the number the
kernel prints in its banner and the number the distribution disks in
dist/ are named for.  1.4 is the distribution userland - the shell and
the commands in disks/dist, recovered from the original disks.  1.61 is
where the kernel source was recovered from: it is the provenance of that
source and not what the tree now is.

---------------------

Morrow Designs Micronix and tools					updated 30 Sep 2026

directories:

filesystem:
	built by the top level makefile from the distribution disks
	this is used by the usersim to run against
	it holds the source and rebuilds itself natively (cd /usr/src; make)

disks:
	floppy images recovered from the net, version 1.4 and 1.3

wslib:
	the whitesmith's libraries, burst apart and disassembled - gone
	from the top level; what remains is under attic/wslib and
	attic/wssrc, with the rest of the retired material

src/micronix:
	the source tree for things that are to be built natively, including
    libraries, commands and the kernel.
	this is gradually being fleshed out with replacements for the micronix
	utilities that I don't have source for, namely all of them.
	notable additions:
		a v7 make
		the PWB yacc, lex, expr and fd2, and 2.11's awk
		tar, msh (the shell), less, diff and mv
		an in-memory, ansi-only, vi subset derived from stevie
		2.11's ls and cp
		a working pwd, rm, mknod
		the ccc compiler, described below
	
src/micronix/lib:
	additions and replacements for the whitesmith's library.
		
src/micronix/sys:
	kernel source, recovered from a micronix 1.61 build (see the version
	note above), with include files.
	the formatting of the original source was really quirky and archaic,
	so I re-indented it to a more K&R like style.
	it is NOT ansi, and compiles with ccc (see below).

src/micronix/stand:
	ghidra-driven rewrite of the cold boot loaders for the kernel

src/micronix/include:
	include files rejiggered to make porting from v6 and v7 easier

src/tools:
	file system checkers, dumper and extractor
	object file tools, including an overachieving nm and 
	a rootin' tootin' fire-breathing disassembler that knows about
	hitech objects, whitesmith's objects, and com files, does
	code tracing, and allows a symbol file to be fed in. 

src/lib:
	libraries for file system, disassembly, and random utility
	
src/usersim:
	micronix user mode simulator, including upm, the cp/m emulator.
	it is quite solid.

	special files are symlinks that name the device - bdev(maj,min) or
	cdev(maj,min) - and the block devices are real: mount() and umount()
	go through the host fslib, with -D <devdir> pointing at a directory of
	bdev(maj,min) links to images.  mkfs, fsck, icheck and the rest open
	the raw device the same way.

	build it on any random unix box (centos is baseline),
	and run:  sim

src/hwsim:
	a hardware level mpz80 simulator that boots the micronix kernel.
	it includes the ability to load+run the monitor roms, load symbol
	tables, has an ICE-like debugger with breakpoints, single step,
	disassembler, and so on.  furthermore, it has a modular architecture
	that allows plugging in different chip simulators.

	it runs cp/m and boots micronix to a shell, with interrupt controller,
	trapping, memory mapping, and disk reading and writing for the five
	disk controllers (djdma, hddma, hdca, ide and ncr5380).  the card list
	is the DRIVERS line in src/hwsim/d1/Makefile.

	there's a means for importing and exporting data to cp/m via the
	inp: and out: devices in pip, so hex files can be shipped to get
	programs in and out.

	the djdma simulator reads IMD files directly, and writes produce
	a delta file that is loaded at the next startup, so there's no
	modification of the original IMD.  the imd utility can generate
	a merged IMD file that contains any changes.

src/include:
	library include files for the emulation

ccc:
	this tree's own c compiler, written from scratch - cpp, c0, c1, peep,
	asz and ld, the passes in src/micronix/libexec and the driver and
	friends in src/micronix/cmd.  it cross builds on the host (mxccc) and
	natively inside micronix (ccc), and targets both micronix and cp/m -
	-m micronix links libu, -m cpm links libcpm, and both link the one
	pure libc.  everything in this tree builds with it.

extra/v6, extra/v7, extra/2.11
	oh, yeah.  this is the real mc-coy.  this is useful for reference and
	tool source grabbing.  the porting to micronix is simple, if tedious.
	the include files are subtly different.

extra/docs:
	almost everything I could find on the micronix hardware, and miscellaneous
	morrow stuff that may be useful.

extra/decomp:
	a decompiler that knows about code flow, system calls, and
        with the goal of generating recompilable C.  very much a WIP.
	probably throw away now that ghidra exists.

extra/sim:
	a bunch of 8 bit simulators for cribbing ideas/code from.
	these all are licensed by thier original authors, so...

running micronix in simulation
------------------------------

there are two simulators, and they are two different experiences.

usersim (src/usersim) is the easy one - a user-mode simulator that hosts
the micronix system calls on linux, no simulated hardware.  it runs
against filesystem/, the tree's own native build, and its block devices
are real: mount(), mkfs and fsck all go through the host fslib, with
-D <devdir> mapping bdev(maj,min) to disk images.  build once, then:

	make
	src/usersim/sim

	# or, for a quite strange experience,
	src/usersim/sim bin/man sh | less

hwsim (src/hwsim/d1) is the real machine - a hardware-level simulator of
the Morrow MPZ80: the monitor rom, the real kernel, and all five disk
controllers.  it boots micronix to a real single-user '#' shell on a
real (simulated) disk, and it is the one that needs setup.

the short boot, from a standing start:

	make
	cd src/micronix && make bootstrap
	make -C src/micronix/stand/boot bootimgs

	disks/hdinstall/create_vol myvol	# a bootable volume
	disks/hdinstall/boot_vol myvol		# boot it, console in an xterm

boot_vol passes -c 0x70c: boot the hdcdma straight, skip the monitor,
and put the console in an xterm of its own.  the boot block mkfs left on
cylinder 0 loads /micronix, and the shell prompt is '#'.

the performance build is the same simulator with the debug machinery
compiled out - d1p, or d1 -F - and it runs at roughly a 30 mhz z80,
which makes it pleasant to use rather than watch.  everything works at
that speed: multi-user logins, a compile on one terminal while ps runs
on another.

the full install, from the distribution - build the diskettes, provision
a two-disk machine, and install it automatically:

	dist/create_dist 8		# 9 diskettes into dist/mx2.0-8-*.IMD
	disks/hdinstall/build_dist	# two 34 MB volumes + the install script
	disks/hdinstall/dist_boot	# boot and install all 9, watch the xterm

build_dist and dist_boot drive the install through /dev/mediachg, the
out-of-band media changer: the guest program chg swaps the floppy images
in the simulator without an eject syscall, and the install script loads
and copies each diskette in turn.  the grammar is
src/micronix/sys/MEDIACHG.md.

the whole flow, the disk-label model and the boot loaders are in
docs/INSTALLATION.md, disks/hdinstall/README and sys/DISKLABEL.md.

the provisioning tool
---------------------

a command-line tool to drive hwsim the way the per-card scripts now do
by hand.  one sim.json holds every simulator definition, and the tool
runs a sim's whole lifecycle with verbs instead of four near-identical
shell scripts:

	hwsim format <sim>	mnix initialize (geometry)
	hwsim partition <sim>	mnix label (slices)
	hwsim mkfs <sim>		mnix mkfs (filesystem + boot)
	hwsim boot <sim>		launch d1/d1p from the config + volumes
	hwsim add/del <sim>	a volume in or out

the abstractions behind it:

- a sim is a collection of volumes.  a volume is a disk on a controller -
  its geometry and its card, not a role.
- root and swap are the kernel's setdev arguments, not volume attributes.
- a device is controller:unit<slice> (hdcdma:0a); the major numbers are
  fixed and the minor is (slice<<5)|unit, so the json never stores one.
- -c is one number packing four things; the json holds them named:
  boot (hdcdma/djdma/hdca), nomon, xterm mask, clock.

the common case - build and boot the sample two-disk sim (q540-dual):

	hwsim format q540-dual		# initialize both m40 volumes
	hwsim partition q540-dual	# the default slice table
	hwsim mkfs q540-dual		# filesystem + boot on slice a
	hwsim boot q540-dual		# d1p, packed -c, both disks wired

the full design - the json schema and the bit tables - is
src/tools/PROVISION.md.  build_dist and dist_boot are the same thing done
by hand, and the media changer (sys/MEDIACHG.md) is how a boot's floppies
get loaded.

this github is prettily referenced in my cybernecromancy site:

https://retro.zen-room.org/morrow-micronix/user-mode-simulator

