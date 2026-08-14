;	strcat(dest, src) - append src to dest, return dest
;
;	The old body walked the destination in BC, which is the
;	caller's register-variable home and was never saved.  This one
;	reads its arguments where they sit instead of popping them, and
;	touches only A, DE and HL - the registers a callee may scratch.
;	The starting destination, which is the return value, waits on
;	the stack instead of in a register.
	psect	text
	global	_strcat

_strcat:
	ex	de,hl		;de = destination, which arrived in hl
	ld	hl,2
	add	hl,sp		;past the return address
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a		;hl = source
	push	de		;the return value, kept off the registers

;	find the destination's terminator
1:	ld	a,(de)
	or	a
	jr	z,2f
	inc	de
	jr	1b

;	copy the source, terminator and all
2:	ld	a,(hl)
	ld	(de),a
	or	a
	jr	z,3f
	inc	de
	inc	hl
	jr	2b

3:	pop	hl		;the destination we were given
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
