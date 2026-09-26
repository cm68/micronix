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
	.global	start, sexit, _enterk
	.extern	_main, _inumber, _loadbase

	.text
start:
	call	_main
	jp	sexit
sexit:
	jp	0

;
; enterk - jump to the kernel with its own inode number in HL.
;
; The loader picked a file and loaded it, and the kernel that file turned
; out to be wants to open that same file again - its overlays ride in it -
; so the loader has to say which inode it came from.  There is no way to
; say "in HL" in C, which is the same reason start and sexit are in this
; file rather than in boot.c.
;
; A register and not a cell: the loader and the kernel are separately
; built programs, and a register is a one-instruction agreement at the one
; place they meet, where a fixed address would be a constant the two have
; to keep in step.  IX carries the target because jp (hl) is the only
; register-indirect jump that takes HL, and HL is carrying the payload.
;
; HL zero means "no inode" - see boot.c's inumber, which starts at 1, that
; being the root, so a boot path that chose nothing must not look like one
; that chose the root.  The kernel's boot: stashes it in _kino.
;
; The underscore, unlike start and sexit above: those are reached by
; address - mwboot1 jumps to the text base - and this one is called from
; C, which is what spells it _enterk.
;
_enterk:
	ld	ix,(_loadbase)
	ld	hl,(_inumber)
	jp	(ix)

; vim: tabstop=4 shiftwidth=4 noexpandtab:
