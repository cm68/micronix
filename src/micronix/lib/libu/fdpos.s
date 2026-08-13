;
; file position tracking for lseek
;
; micronix seek() does not return the old file position,
; so lseek() must track the position of every open file
; in user space.  the read and write wrappers call fdadd
; to advance the position, open and creat call fdclr to
; reset it, and dup calls fdcpy to copy it.
;
; _fdpos is a table of 16 32-bit positions, one per fd
; (micronix allows 16 open files per process).
;
; limits: positions of fds inherited across fork or exec
; are not known, and offsets shared through dup diverge
; once either fd moves.  seek() is a wrapper over lseek
; (seek.c), so both calls keep the tracking current; only
; the internal seekraw stub bypasses it.
;
	.global __fdpos
	.global fdadd
	.global fdclr
	.global fdcpy

	.text

;
; fdadd: a = fd, hl = byte count
; adds count to _fdpos[fd]; returns count in hl
;
; A long is stored HIGH word first - see QLONG.md - so the half this
; addition starts from is two bytes in, and the carry out of it goes
; DOWN into the bytes before it rather than up into the ones after.
;
; Added the other way round, the position reached lseek with its
; halves swapped: a file read to its end at 31 bytes was 0x001f0000,
; and ftell, which is that value less what is still in the buffer,
; answered 0x001effe5 where it meant 4.  Everything that seeks back
; over what it has written is on this - asz assembles into a temp
; file and copies it back out - and nothing fails loudly: the seek
; simply lands somewhere else.
;
; INC and DEC on a register pair leave the flags alone, and so do
; LD A,(HL) and LD (HL),A, so the carry survives the walk back down.
;
fdadd:
	push	hl
	add 	a,a
	add 	a,a		; fd * 4
	ld 	e,a
	ld 	d,0
	ld 	hl,__fdpos
	add 	hl,de
	pop 	de		; count
	inc 	hl
	inc 	hl		; the low word, two bytes in
	ld 	a,(hl)
	add 	a,e
	ld 	(hl),a
	inc 	hl
	ld 	a,(hl)
	adc 	a,d
	ld 	(hl),a
	dec 	hl
	dec 	hl
	dec 	hl		; and the high word, back at the front
	ld 	a,(hl)
	adc 	a,0
	ld 	(hl),a
	inc 	hl
	ld 	a,(hl)
	adc 	a,0
	ld 	(hl),a
	ex 	de,hl		; return count in hl
	ret

;
; fdclr: a = fd
; zeroes _fdpos[fd]; preserves hl
;
fdclr:
	push	hl
	add 	a,a
	add 	a,a		; fd * 4
	ld 	e,a
	ld 	d,0
	ld 	hl,__fdpos
	add 	hl,de
	xor 	a
	ld 	(hl),a
	inc 	hl
	ld 	(hl),a
	inc 	hl
	ld 	(hl),a
	inc 	hl
	ld 	(hl),a
	pop 	hl
	ret

;
; fdcpy: a = source fd, l = destination fd
; copies _fdpos[source] to _fdpos[dest]; preserves hl
;
fdcpy:
	push	hl
	push	bc
	ld 	b,a		; save source fd
	ld 	a,l
	add 	a,a
	add 	a,a		; dest fd * 4
	ld 	e,a
	ld 	d,0
	ld 	hl,__fdpos
	add 	hl,de
	ex 	de,hl		; de = dest entry
	ld 	a,b
	add 	a,a
	add 	a,a		; source fd * 4
	ld 	c,a
	ld 	b,0
	ld 	hl,__fdpos
	add 	hl,bc		; hl = source entry
	ld 	bc,4
	ldir
	pop 	bc
	pop 	hl
	ret

	.bss
__fdpos:	.ds 64

; vim: tabstop=8 shiftwidth=8 noexpandtab:
