;	32-bit and: HL':HL &= DE':DE
;
;	See QLONG.md for the convention.  There is no 16-bit and, so both
;	halves go through A a byte at a time; the exx does for the high
;	words what it does everywhere else.  Signed and unsigned are the
;	same operation.

	psect	text
	global	qand

qand:
	ld	a,e
	and	l
	ld	l,a
	ld	a,d
	and	h
	ld	h,a
	exx
	ld	a,e
	and	l
	ld	l,a
	ld	a,d
	and	h
	ld	h,a
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
