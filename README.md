for a blast from the past, type:

 make test

 you now are running micronix 1.4 shell and can do a lot

---------------------

Morrow Designs Micronix and tools

directories:

filesystem:
	the filesystem.dist, with local edits for functionality.
	the eventual goal is to have this directory a self-building tree
	with source

filesystem.dist:
	recovered filesystems from images contained in the images directory.
	produced by the tools/copyall program

src/fstools:
	file system checkers, dumper and extractor

src/sgs:
	object file tools, including an overachieving nm
	
tools:
	other random work in progress. including a very flexible disassembler

images:
	floppy images recovered from the net, version 1.4 and 1.3

kernel:
	recovered kernel source for micronix 1.61

include:
	recovered include files 1.61

usersim:
	micronix user mode simulator mostly works, including upm, the cp/m
	emulator.  some of the system calls are still not real (mknod!?)

	build it on any random unix box (centos is baseline),
	and run:  sim
	
hitechc:
	the hitech c compiler.  this is not capable of running yet, but if the
	cp/m libc gets removed via binary jiggery-pokery, and replaced with
	a z80, micronix one, then we are in business with the most modern native
	c compiler in existence.

qc:
	plan B, if the hitech c effort is too large.  this compiler is a bit lame,
	as it is small-c derived, so args get pushed wrong, no ansi, etc. 
	but it is native, and it is source.

v6, v7:
	oh, yeah.  this is the real mc-coy.  this is useful for reference and
	tool source grabbing.  the porting to micronix is simple, if tedious.
	the include files are subtly different.

docs:
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

decomp:
	a decompiler that knows about code flow, system calls, and
        with the goal of generating recompilable C.  very much a WIP.
	probably throw away now that ghidra exists.

fullsim:
	the beginnings of a mpz80 simulator.  WIP.

sim:
	a bunch of 8 bit simulators for cribbing ideas/code from.
	these all are licensed by thier original authors, so...

make the kernel compile - this is unix v6 source code, not a hint
of ansi.  luckily, the kernel source is k+r version 7 C.

replace all the missing utility and application source with the
sources from actual unix version 6.  this will require a lot of
textual stuff like getting rid of =- and =+, and fixing initializers.

get an object code improver that will fix the pretty bad stuff that
whitesmith's generates. 

this github is prettily referenced in my cybernecromancy site:

https://retro.zen-room.org/morrow-micronix/user-mode-simulator

