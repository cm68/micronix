;
; The overlay slot: the one page of kernel text a driver module is
; stamped into.  sys/ovl.h is the frame's description; this is its
; other end, the place in the kernel the frame occupies.
;
; A kernel is linked with no disk drivers and this page where they
; would be, and setdev copies one module's page over it at install
; time (cmd/setdev/setdev.c).  What it copies is a page whose first
; bytes are the module's header, so after the copy OVLBASE holds the
; header and the driver behind it, and _start() registers it through
; ovlattach() like any other module.  The kernel does not know which
; driver that is; the module says so.
;
; Why a page of text and not a hole in the address space: the loader
; writes the text section, then the data section, and a gap between
; them is a gap it would have to be taught about.  Declared as text,
; the slot is loaded by the loader that already runs, at the address
; it is already linked at - and because it is the last of the text,
; the data segment follows it with nothing between them.
;
; The address is not written here.  The GNUmakefile links this object
; with -Ttext=$(OVLBASE), so the object's own header records the
; page, and mxld's final link places it there rather than packing it
; after the resident text the way an ordinary object would.  That is
; the same mechanism win.o and upage.o use for their pages, arrived
; at from the text side.
;
; The assembler's .align is no use here: it counts from the start of
; the section it is assembling, and mxld decides where that section
; starts, so the one is a property of the link and not of this file.
;
	.text
	.globl	_ovlslot
_ovlslot:
	.ds	4096

;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;
