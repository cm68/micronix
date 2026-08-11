;	max(a, b) - the larger of two unsigned words
;
;	The old body parked the return address in BC, the caller's
;	register-variable home, and never gave it back.  This one reads
;	the arguments where they sit and touches only A, DE, HL and the
;	flags.
	global	_max
	psect	text

_max:
	ld	hl,2
	add	hl,sp		;past the return address
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		;de = arg 1
	inc	hl
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a		;hl = arg 2
	push	hl		;save it
	or	a		;clear carry
	sbc	hl,de		;compare
	pop	hl		;restore
	ret	nc		;return if greater or equal
	ex	de,hl		;otherwise return the other
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
