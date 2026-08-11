	psect	text
	global	_randomiz, _srand

_randomiz:
	ld	a,r
	ld	l,a
	ld	h,0
	push	hl
	call	_srand
	pop	hl
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
