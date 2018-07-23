# micronix


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



TODO:

make the kernel compile - this is unix v6 source code, not a hint
of ansi.  luckily, the kernel source is k+r version 7 C.

replace all the missing utility and application source with the
sources from actual unix version 6.  this will require a lot of
textual stuff like getting rid of =- and =+, and fixing initializers.

get an object code improver that will fix the pretty bad stuff that
whitesmith's generates.  

