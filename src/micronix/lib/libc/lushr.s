;	logical (unsigned) long right shift
;	value in HLDE, count in B

	global	lushr, ulrsh
	psect	text

lushr:
ulrsh:
	ld	a,b		;check for zero shift
	or	a
	ret	z
	cp	33
	jr	c,1f		;limit shift to 32 bits
	ld	b,32
1:
	srl	h		;logical shift (unsigned)
	rr	l
	rr	d
	rr	e
	djnz	1b
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
