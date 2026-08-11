	psect	text
digit:	sub	'0'
	ret	c
	cp	10
	ccf
	ret

	global	_atoi
_atoi:	pop	hl	;return address - hl, not bc: bc is the
	pop	de		;caller's register variable and this uses
	push	de		;it as a multiply scratch below, so it has
	push	hl		;to be saved, and it cannot be saved after
	push	bc		;something has already overwritten it
	ld	hl,0
1:
	ld	a,(de)
	inc	de
	cp	' '
	jr	z,1b
	cp	'	'	;tab
	jr	z,1b
	dec	de		;point to 1st non blank char
	cp	'-'
	jr	z,3f
	cp	'+'
	jr	nz,2f
	or	a		;reset zero flag
3:
	inc	de
2:	ex	af,af'
1:
	ld	a,(de)
	inc	de
	call	digit
	jr	c,3f
	add	hl,hl
	ld	c,l
	ld	b,h
	add	hl,hl
	add	hl,hl
	add	hl,bc
	ld	c,a
	ld	b,0
	add	hl,bc
	jr	1b

3:
	ex	af,af'
	jr	nz,4f
	ex	de,hl
	ld	hl,0
	sbc	hl,de
4:
	pop	bc		;the caller's, back
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
