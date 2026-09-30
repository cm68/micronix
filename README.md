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

running the hardware simulator, from a standing start
----------------------------------------------------

you can make a hard disk image and boot micronix without any blessed
snapshot, starting from the kernel source.  docs/INSTALLATION.md has
the whole flow; the short of it:

	# build the compiler, the libraries, the commands and the kernel
	make
	cd src/micronix
	make bootstrap

	# make a bootable volume and boot it - disks/hdinstall/README has
	# the detail
	disks/hdinstall/create_vol myvol
	disks/hdinstall/boot_vol myvol

-B says boot straight from the hdcdma controller, skipping the monitor;
the boot block that mkfs put on cylinder 0 loads /micronix.  the shell
prompt is '#'.

this github is prettily referenced in my cybernecromancy site:

https://retro.zen-room.org/morrow-micronix/user-mode-simulator

