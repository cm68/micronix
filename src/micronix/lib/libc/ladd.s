	psect	text
	global	aladd, lladd, ladd

aladd:
lladd:
ladd:
	exx
	pop	hl
	exx
	pop	bc
	ex	de,hl
	add	hl,bc
	ex	de,hl
	pop	bc
	adc	hl,bc
	exx
	push	hl
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
