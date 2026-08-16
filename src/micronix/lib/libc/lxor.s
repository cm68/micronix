	psect	text
	global	alxor, llxor, lxor

alxor:
llxor:
lxor:
	exx
	pop	hl
	exx
	pop	bc
	ld	a,c
	xor	e
	ld	e,a
	ld	a,b
	xor	d
	ld	d,a
	pop	bc
	ld	a,c
	xor	l
	ld	l,a
	ld	a,b
	xor	h
	ld	h,a
	exx
	push	hl
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
