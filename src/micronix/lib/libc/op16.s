;	16-bit logical operations for code generator
;	HL = HL op DE, result in HL

	psect	text
	global	or16, xor16, and16, com16

; HL = HL | DE
or16:
	ld	a,l
	or	e
	ld	l,a
	ld	a,h
	or	d
	ld	h,a
	ret

; HL = HL ^ DE
xor16:
	ld	a,l
	xor	e
	ld	l,a
	ld	a,h
	xor	d
	ld	h,a
	ret

; HL = HL & DE
and16:
	ld	a,l
	and	e
	ld	l,a
	ld	a,h
	and	d
	ld	h,a
	ret

; HL = ~HL (one's complement)
com16:
	ld	a,l
	cpl
	ld	l,a
	ld	a,h
	cpl
	ld	h,a
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
