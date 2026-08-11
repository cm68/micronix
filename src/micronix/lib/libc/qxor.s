;	32-bit exclusive or: HL':HL ^= DE':DE
;
;	See QLONG.md for the convention and qand.s for the shape.

	psect	text
	global	qxor

qxor:
	ld	a,e
	xor	l
	ld	l,a
	ld	a,d
	xor	h
	ld	h,a
	exx
	ld	a,e
	xor	l
	ld	l,a
	ld	a,d
	xor	h
	ld	h,a
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
