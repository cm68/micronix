;	32-bit complement: HL':HL = ~HL':HL
;
;	See QLONG.md for the convention.  One operand, so DE':DE is not
;	involved and comes through untouched.

	psect	text
	global	qcom

qcom:
	ld	a,l
	cpl
	ld	l,a
	ld	a,h
	cpl
	ld	h,a
	exx
	ld	a,l
	cpl
	ld	l,a
	ld	a,h
	cpl
	ld	h,a
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
