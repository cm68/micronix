# micronix
#
# for a blast from the past, go to usersim and type make
# finally, run sim.
# you now are running micronix 1.4 shell and can do a lot
#

Morrow Designs Micronix and tools

directories:

filesystem:
	recovered filesystems from images contained in the images directory.
	produced by the tools/copyall program

tools:
	file system checkers and object file tools, including a very flexible disassembler

images:
	floppy images recovered from the net, version 1.4 and 1.3

kernel:
	recovered kernel source for micronix 1.61

include:
	recovered include files 1.61

usersim:
	Big news:  as of july 23 2018,
	micronix user mode simulator mostly works.
	build it on any random unix box (centos is baseline),
	and run:  sim

	date, ls. man, and so on work pretty well.
	
	as of Aug 1, almost everything works.

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
        with the goal of generating recompilable C

TODO:

make the kernel compile - this is unix v6 source code, not a hint
of ansi.  luckily, the kernel source is k+r version 7 C.

replace all the missing utility and application source with the
sources from actual unix version 6.  this will require a lot of
textual stuff like getting rid of =- and =+, and fixing initializers.

get an object code improver that will fix the pretty bad stuff that
whitesmith's generates.  

