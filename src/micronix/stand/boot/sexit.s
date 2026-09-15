;
; crt0 for the boot loader, linked manually instead of the library's.
;
; micronix/stand/boot/sexit.s
;
; A normal program links crt0.o, whose start() reads argc/argv off the
; stack exec() left and calls main(argc, argv).  The loader is entered
; by mwboot1 jumping straight to its text offset with an empty stack,
; so there is no argc/argv to read: start() here does the one thing
; that matters and calls main with nothing.  Because the loader links
; without crt0.o, the library's exit() and its stdio flush never come
; in - which is what this file, as sexit, used to arrange for alone.
;
; main() does not return: load() jumps to the kernel it loaded.  If it
; did, sexit is the same answer bail() gives - back to the rom at 0.
;
	.global	start, sexit
	.extern	_main

	.text
start:
	call	_main
	jp	sexit
sexit:
	jp	0

; vim: tabstop=4 shiftwidth=4 noexpandtab:
