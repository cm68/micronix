;
; c runtime startup code for micronix
; with hl return value for hitech compiler
;
	.extern sexit
	.extern _main
	.global start

	.text
start:
;
; The bss clearing below is disabled: the linker now folds bss into
; the data segment as zero bytes, and exec() reads those into the
; process, so an uninitialised global reads zero before main() runs.
; Left in, commented out, because it is the one place that records
; what the original arrangement was and why.
;
; Nothing used to do it - exec did not clear bss, and this code only
; built argc and argv - so an uninitialised global held whatever was
; in the memory, while C is written on the understanding that it reads
; zero.  mkfs found it: a block number it had not assigned yet read
; back as 58626, and it went looking for that block on a drive that
; has 10404 of them.
;
; __Lbss and __Hbss are absolute symbols - the linker makes their value
; the address, so these are immediate loads.  Written (__Lbss) first,
; which fetches the word living at that address instead: the first word
; of the bss being cleared, garbage, and the loop then ran away over
; the program and hung before main printed anything.
;
;		ld	hl,__Lbss
;		ld	de,__Hbss
;bssclr:
;		ld	a,l
;		cp	e
;		jr	nz,bsszap
;		ld	a,h
;		cp	d
;		jr	z,bssdone
;bsszap:
;		ld	(hl),0
;		inc	hl
;		jr	bssclr
;bssdone:

	pop	de	; this is argc
	ld	hl,0	; load argv
	add	hl,sp
	push	hl	; argv, the second argument, on the stack
	ex	de,hl	; argc, the first, rides in hl
	call	_main	; call main
	push	hl	; return value
	jp	sexit	; no-return entry of exit(): takes bare
			; [status] with no return-address slot

	.data
;
; these symbols are filled out by the linker
;
__Htext::	.dw	0
__Ltext::	.dw	0
__Hdata::	.dw	0
__Ldata::	.dw	0
__Hbss::	.dw	0
__Lbss::	.dw	0

; vim: tabstop=4 shiftwidth=4 noexpandtab:
