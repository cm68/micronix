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
		a much better make, 
		an in-memory, ansi-only, vi subset derived from stevie
		2.11's ls and cp
		a working pwd, rm, mknod
		a cc that deals with whitesmith's and hitech when that compiler
		is ready
	
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

src/hitechc:
	the hitech c compiler.  this is not capable of running yet, but a big
	start has been made to replace the bottom level i/o with micronix versions.
	this is facilitated by a binary patch tool that effectively pattern
	matches library fragments and patches on top of them.   it's good
	enough to work for cpp already, but the compiler passes use a different
	library implementation.
	when that's done, then we are in business with the most modern native
	c compiler in existence.

extra/qc:
	plan B, if the hitech c effort is too large.  this compiler is a bit lame,
	as it is small-c derived, so args get pushed wrong, no ansi, etc. 
	but it is native, and it is source.

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

TODO:

replace all the missing utility and application source with the
sources from actual unix version 6.  this will require a lot of
textual stuff like getting rid of =- and =+, and fixing initializers.

get an object code improver that will fix the pretty bad stuff that
whitesmith's generates. 

this github is prettily referenced in my cybernecromancy site:

https://retro.zen-room.org/morrow-micronix/user-mode-simulator

