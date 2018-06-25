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


TODO:

make whitesmith's c run on some unix simulator - zxcc 5.7 doesn't work

make the kernel compile - this is unix v6 source code, not a hint
of ansi.  luckily, the kernel source is k+r version 7 C.

replace all the missing utility and application source with the
sources from actual unix version 6.  this will require a lot of
textual stuff like getting rid of =- and =+, and fixing initializers.

get an object code improver that will fix the pretty bad stuff that
whitesmith's generates.  

for example, 
	static unsigned char uc;
	uc++;

generates:

	ld a,(uc)	; 10 bytes
	add a,01
	ld (uc),a

what?  how about:

	ld hl,uc	; 4 bytes
	inc (hl)

