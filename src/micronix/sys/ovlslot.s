;
; The overlay slot: the one page of kernel memory a driver module is
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
; The page is data, not text, because it is parked above the buffer
; pool at 0xd000 (segment OVLSEG), just below the buffer window; the
; scratch window is at 0xc000 below it.  The text and the resident data
; run 0x1000 to _ebss, and the driver page is a fixed data page the way
; win.o and upage.o are.  The loader writes it with the rest of the data
; section, and setdev overwrites it.
;
; The address is not written here.  The GNUmakefile links this object
; with -Sdata -Tdata=$(OVLBASE), so the object's own header records the
; page, and mxld's final link places it there.
;
	.data
	.globl	_ovlslot
_ovlslot:
	.ds	4096

;
; vim: tabstop=4 shiftwidth=4 noexpandtab:
;
