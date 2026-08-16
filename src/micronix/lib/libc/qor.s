;	32-bit or: HL':HL |= DE':DE
;
;	See QLONG.md for the convention and qand.s for the shape.

	psect	text
	global	qor

qor:
	ld	a,e
	or	l
	ld	l,a
	ld	a,d
	or	h
	ld	h,a
	exx
	ld	a,e
	or	l
	ld	l,a
	ld	a,d
	or	h
	ld	h,a
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
