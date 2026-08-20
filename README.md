for a blast from the past, type:

 make test

 you now are running micronix 1.4 shell and can do a lot
 including: (this builds the recovered 1.61 kernel)

	cd /usr/src/sys
	make

or, for a quite strange experience,

    src/usersim/sim bin/man sh | less

	(run the simulated z80 micronix man program on sh, and pipe it to linux less)

---------------------
updated 7 July 2023

the whitesmith's c compiler for cp/m has been released in source form by PJ Plauger,
and although it is quite a long way from usable in the released form on micronix,
I have started porting it to micronix

also, the bizarre anat assembler that is a baneful attempt to make a high level
language out of 8080 is also in the source that is released.  As I have no real
desire to learn to read that horrible stuff, I have snagged a relocating assembler
that consumes standard z80 mnemonics call TRASM, and hacked it to bits.
the author, Gavin Tersteeg, gctersteeg@gmail.com, built an impressive tool kit.
in this source tree, it's call asz, and it will be the backend to the improved
whitesmith compiler.

---------------------

Morrow Designs Micronix and tools					updated 6 Sep 2021

directories:

filesystem:
	built by the top level makefile from the distribution disks
	this is used by the usersim to run against
	the eventual goal is to have this directory a self-building tree
	with source

disks:
	floppy images recovered from the net, version 1.4 and 1.3

wslib:
	the whitesmith's libraries, burst apart and disassembled

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
	recovered kernel source for micronix 1.61, with include files
	the formatting of the original source was really quirky and archaic,
	so I re-indented it to a more K&R like style.
	it is NOT ansi, and compiles on whitesmith's C.

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
	micronix user mode simulator mostly works, including upm, the cp/m
	emulator.  some of the system calls are still not real and always
	fail.  however, you will find it quite solid. 

	however, I used an interesting method (hack, really)
	to fake out special files.  special files are a symlink containing
	a string of the form:  [cb]dev(<decimal major>,<decimal minor>)
	you can then actually create file named this, and mkfs, fsck, etc
	inside the usersim will actually think they are dealing with a bdev
	and be reading and writing the image file.  eventually,
	I'll implement mount and interface the fslib to the usersim.

	build it on any random unix box (centos is baseline),
	and run:  sim

src/hwsim:
	most of hardware level mpz80 simulator capable of running the
	micronix kernel.  it includes the ability to load+run the monitor
	roms, both version 4.47 and 3.75, load symbol tables, has an ICE-like
	debugger with breakpoints, single step, disassembler, and so on.
	furthermore, it has a modular architecture that allows plugging in 
	different chip simulators.

	cp/m works well, and micronix is getting very close, with interrupt
	controller, trapping, memory mapping, disk reading and writing for
	all 3 controllers (djdma, hdc-dma, and hdca).

	there's a means for importing and exporting data to cp/m via the
	inp: and out: devices in pip, so hex files can be shipped to get
	programs in and out.

	the djdma simulator reads IMD files directly, and writes produce
	a delta file that is loaded at the next startup, so there's no
	modification of the original IMD.  the imd utility can generate
	a merged IMD file that contains any changes.

	finally, I've started on a skeleton for other platforms like compupro.

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

compiler woes:

	it turns out that the whitesmith's C compiler is very lame in one
	important way:  BSS symbols never get allocated in the object file.
	that means that code like

	int foo;
	bar() { foo = 9; }

	does not link.  this is craptastic beyond belief.
	the only workaround is to modify the source to move foo to data
	by giving an explicit = 0;

	accordingly, I'm porting a compiler that does not have this lossage.
	software toolworks c/80 is the only reasonably complete native compiler
	I have found.   porting it is non-trivial.

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
snapshot, starting from the kernel source.  m16 is the largest volume
with a reliable kernel.

	# build the host cross-tools (once)
	make hostcc				the compiler: mxccc, mxasz, mxld
	make -C src/tools			mnix, the image reader/writer
	make -C src/micronix/stand/boot		the boot blocks, bootimg-m16

	# build the kernel.  sys is three overlays at a fixed base, so the
	# cross build does not link it - build "unix" with the host tools:
	#	compile each .c:	bin/mxccc -m micronix -O -i../include -c foo.c
	#	assemble each .s:	bin/mxccc -m micronix -c foo.s
	#	link:			bin/mxld -r -Ttext=0x1000 -L lib -lccc -lc -o unix *.o
	# (the native sys/Makefile has the full object list) - ~49k.

	# make the m16 disk
	src/tools/mnix initialize m16 disks/hdinstall/hddma-0
	src/tools/mnix mkfs -i src/micronix/stand/boot/bootimg-m16 \
		disks/hdinstall/hddma-0
	bin/setdev unix 3/8 0/0		# root 3/8 (m16), swap nodev
	src/tools/mnix -f disks/hdinstall/hddma-0 write unix /micronix

	# boot it
	cd src/hwsim/d1
	./d1 -B hdcdma -d ../../disks/hdinstall hdcdma0:hddma-0

-B says boot straight from the hdcdma controller, skipping the monitor;
the boot block that mkfs put on cylinder 0 loads /micronix.  the shell
prompt is '#'.

TODO:

replace all the missing utility and application source with the
sources from actual unix version 6.  this will require a lot of
textual stuff like getting rid of =- and =+, and fixing initializers.

get an object code improver that will fix the pretty bad stuff that
whitesmith's generates. 

this github is prettily referenced in my cybernecromancy site:

https://retro.zen-room.org/morrow-micronix/user-mode-simulator

