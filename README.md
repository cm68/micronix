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

micronix 2.0 is a substantially enhanced kernel, not the 1.61 kernel
with a new number on it.  the boards are discovered at boot and each
driver is a module placed only if its card answers, so one kernel boots
the SS1, the Wunderbus and the MultIO; the buffer cache hashes its
headers and keeps the data in segments behind a window, with room for
over a hundred buffers against 1.61's fifteen; the task maps are
per-process and a switch no longer copies one, and each process has its
own u page and stack; exec builds its arguments straight into the new
image, and execve adds an environment; and the system call table has
grown dup2, ftruncate, ioctl, reboot, execve and select.
src/micronix/sys/FEATURES-2.md is the full account, with the counts
taken from the two kernels.

three additions since the release notes are worth calling out.  rshd is
the big one: the machine can now be logged into from the network and
not only copied from - an rsh from another machine reaches a shell on a
pty, authenticated by /etc/hosts.equiv and ~/.rhosts, so micronix is no
longer only a client.  ioctl now reaches character devices as well as
block ones: a terminal's status is the two requests TIOCGETA and
TIOCSETA, so gtty and stty are at last only library wrappers over them,
and the network card's socket face is its own NET* requests.  select is
there too - crippled but functional: it waits on a mask of up to
sixteen descriptors and returns the ones that can be read, with no
timeout and no write set, and sees only ttys and the network sockets; a
deadline is an alarm(2), which returns it with EINTR.  with these,
micronix is close to seventh edition.  the difference that stands out
is the inode, which keeps two timestamps, access and modify, where V7
keeps a third, change time.

micronix 2.0 - release notes, 21 oct 2026

this is the first release of micronix in 42 years.  it is a complete
source release, with significantly enhanced functionality and much
higher performance on the same hardware:

	an entirely new C compiler and tool chain.  it exploits the Z80
	chip for significant speed and space improvements over the
	Whitesmith's compiler.  in addition, it accepts most of ANSI C.

	up to 128K of ram is used as a buffer cache, greatly reducing
	disk i/o.

	it exploits the mpz80 hardware contexts to make context switches
	much cheaper.

	exec now allows a large number of arguments, limited by memory
	size.

	unix 7th edition changes: ioctl, lseek, dup2, O_APPEND,
	ftruncate, environment variables, bourne shell, make, less and
	many others.

	a network interface has been added supporting an ESP32 connection.
	the ESP32 runs tcp/ip and wifi; rcp copies files over rsh, rshd
	serves rsh into the box, and nd is a network disk on the same
	card - a block device that can serve a volume with no local
	controller.

	the console is chosen at boot: the morrow multio ACE or the compupro
	SS1's 2651.  wunderbus ACEs and the serial lines give additional
	terminals, so the machine is multi-user over serial.

	halt and reboot commands are added.

	format utilities running natively for all supported disk
	controllers.

	modular device drivers: all block device drivers share a single
	4k segment, so many different drivers can be loaded on the same
	kernel.

	automatic interrupt probing: all disk controllers can be strapped
	to any unused S100 VI line, and it will be automatically
	detected.

	automatic detection of interrupt controllers: the morrow multio,
	wunderbus I/O and compupro system support are all supported.

	the tool chain is self-hosting: the source tree rebuilds itself
	natively, and the second generation is byte-identical to the first
	(make verify).

	drivers for the following controllers have been added, along with
	boot rom support if running the included mon5.0:

		morrow hdc/dma, dj/dma, hdca
		compupro disk1, disk2, disk3
		s100computers.com dual ide, scsi

	disk partitioning.  micronix only used up to 32MB of a disk.  a
	new disk labeling facility made disks self-describing, so the
	kernel no longer needs to know disk geometry.

the source tree, the build instructions and the 1.4 userland / 1.61
kernel provenance are in this file.  make test runs the recovered 1.4
shell; the d1 simulator and usersim run the system without the hardware,
and the 2.0 distribution media are under dist/ and attached to the 2.0 release.
the tree is Apache 2.0 (LICENSE); the Z80 instruction tables in
src/include/z80.h carry the zlib/libpng notice in LICENSE.z80.

---------------------

Morrow Designs Micronix and tools					updated 21 Oct 2026

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
	trapping, memory mapping, and disk reading and writing for the disk
	controllers (djdma, hddma, hdca, ide, ncr5380, and CompuPro's
	disk1/disk2/disk3), the ND network card, the SS1 and the MultIO.
	the card list is the DRIVERS line in src/hwsim/d1/GNUmakefile.

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

publicdocs:
	almost everything I could find on the micronix hardware - flattened from
	extra/docs - and miscellaneous morrow stuff that may be useful.

extra/decomp:
	a decompiler that knows about code flow, system calls, and
        with the goal of generating recompilable C.  very much a WIP.
	probably throw away now that ghidra exists.

extra/sim:
	a bunch of 8 bit simulators for cribbing ideas/code from.
	these all are licensed by thier original authors, so...

build prerequisites
-------------------

the tree builds with the host's make and gcc, but nearly all of it is
built for 32 bits: src/GNUmakefile.inc sets MACHINE = -m32, and the
host tools the cross toolchain runs are 32-bit as well.  a host with no
32-bit target and no 32-bit libraries will not build it.  on
debian/ubuntu:

	apt install build-essential gcc-multilib libc6-dev-i386 \
		lib32ncurses-dev bison

	build-essential		gcc, make and the C library
	gcc-multilib		the -m32 target and its runtime
	libc6-dev-i386		32-bit libc headers and crt files
	lib32ncurses-dev	the 32-bit curses the simulators and
				host tools link with -lcurses
	bison			generates src/tools/zmac.c from zmac.y

on a red hat family host the equivalents are gcc, make, glibc-devel.i686,
ncurses-devel.i686 and bison, with the 32-bit packages from a multilib
channel.  centos is the baseline the tree was developed against.

one build is separate from the tree's own.  the ESP32 network card
firmware in src/micronix/stand/nic is a PlatformIO project
(platformio.ini, the arduino framework, the esp32doit-devkit-v1 board);
it is not part of the top level make.  install PlatformIO Core -
"pipx install platformio", or the installer from platformio.org - so
that pio is on PATH, then

	make -C src/micronix/stand/nic		# pio run

the ESP32 tool chain and the arduino framework are downloaded on the
first build.  flashing (make upload) wants the board's USB-UART as
/dev/ttyUSB* or /dev/ttyACM*; on debian/ubuntu that means membership in
the dialout group.

xterm is not needed to build, but the hardware simulator's -c option
puts each simulated terminal in one; without it d1 uses the controlling
terminal.  it is a separate package ("apt install xterm" on
debian/ubuntu) and needs an X display.

running micronix in simulation
------------------------------

two simulators, two experiences.

usersim (src/usersim) is the easy one: it hosts micronix's system calls
on linux and runs against filesystem/, no simulated hardware, no disk
images.  build once, then

	make
	src/usersim/sim

hwsim (src/hwsim/d1) is the real machine - the Morrow MPZ80, the monitor
rom, the real kernel, every controller in its card list - and it boots
micronix to a '#' shell on a simulated disk.  it is the one that needs
setup.

the short boot, from a standing start:

	make
	cd src/micronix && make bootstrap
	make -C src/micronix/stand/boot bootimgs

	disks/hdinstall/create_vol myvol	# a bootable volume
	disks/hdinstall/boot_vol myvol		# boot it, console in an xterm

boot_vol passes -c 0x70c: boot the hdcdma straight, skip the monitor,
and put the console in an xterm of its own.  the boot block mkfs left on
cylinder 0 loads /micronix.

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

a command-line tool, src/tools/hwsim (built by make at the top of the
tree), that drives the simulator the way the per-card scripts do by
hand.  the simulated platforms are defined in sim.json at the top of the
tree: each entry under "sims" names the emulator and kernel, the monitor
and level-1 boot roms, the boot device, the console and clock
configuration, the wiring profile, the root and swap device tokens, and
the volumes - controller, unit, geometry and image for each.  the
"mediasets" in the same file name the floppy IMD sets, and -f names a
different platform file.

a sim is a collection of volumes, and a volume is a disk on a controller
- its geometry and its card, not a role.  the verbs:

	hwsim format <sim>		mnix initialize (geometry label)
	hwsim partition <sim>		mnix label (default slice table)
	hwsim mkfs <sim>		mnix mkfs (filesystem + boot)
	hwsim boot <sim>		launch d1/d1p from config + volumes
	hwsim add <sim> <controller> <unit> <geometry> <image>
	hwsim del <sim> <controller>:<unit>
	hwsim list			the sims defined in the file
	hwsim show <sim>		one sim's definition

root and swap are the kernel's setdev arguments, named as device tokens
(hdcdma:0a), not volume attributes; -c packs boot, nomon, the xterm mask
and the clock from named fields.  the common case - build and boot the
sample two-disk sim (q540-dual):

	src/tools/hwsim format q540-dual	# initialize both m40 volumes
	src/tools/hwsim partition q540-dual	# the default slice table
	src/tools/hwsim mkfs q540-dual		# filesystem + boot on slice a
	src/tools/hwsim boot q540-dual		# d1p, packed -c, both disks wired

the full design - the json schema and the bit tables - is
src/tools/PROVISION.md.  build_dist and dist_boot are the same thing done
by hand, and the media changer (sys/MEDIACHG.md) is how a boot's floppies
get loaded.

this github is prettily referenced in my cybernecromancy site:

https://retro.zen-room.org/morrow-micronix/user-mode-simulator

