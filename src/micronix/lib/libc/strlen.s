;	strlen(s)
;
;	cpir is the instruction for this.  It compares a against (hl),
;	advances hl and counts bc down, repeating until they match or bc
;	runs out - a byte every 21 cycles, where the loop this replaces
;	took 40 to do the same thing by hand:
;
;		ld a,(de) / or a / ret z / inc hl / inc de / jr
;
;	78 + 21n against 36 + 40n, so it pays from about the fourth
;	character and is a third faster by the tenth.  strlen is called
;	107,137 times over one source of ours, most of that in peep and
;	asz, and the tenth character is where those live.
;
;	The string arrives in hl, which is where cpir wants it, so
;	nothing is fetched or shuffled to begin.  bc is the caller's
;	register variable and cpir counts with it, so it is saved.

	psect	text
	global	_strlen

_strlen:
	push	bc		;the caller's register variable
	ld	d,h
	ld	e,l		;where the string started
	xor	a		;the byte being looked for
	ld	bc,0		;65536: longer than anything addressable
	cpir
	;
	;	hl is one past the null, so the length is hl - de - 1.
	;	dec sets no flags, so the or is what clears the carry
	;	for the subtract.
	;
	dec	hl
	or	a
	sbc	hl,de
	pop	bc
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
