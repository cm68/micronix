;	Long (32-bit) complement
;	HLDE = ~HLDE

	psect	text
	global	lcom

lcom:
	ld	a,e
	cpl
	ld	e,a
	ld	a,d
	cpl
	ld	d,a
	ld	a,l
	cpl
	ld	l,a
	ld	a,h
	cpl
	ld	h,a
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
