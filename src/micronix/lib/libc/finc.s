;	lfinc - floating increment
;	lfdec - floating decrement

	psect	text
	global	lfinc, lfdec, asfladd

lfinc:
	exx
	ld	hl,one
incdec:
	exx			;restore original hl
	ld	e,(hl)		;get left operand original value
	inc	hl
	ld	d,(hl)
	inc	hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	bc
	push	de		;save on the stack for 'ron
	dec	hl		;restore pointer value
	dec	hl
	dec	hl
	exx			;now pointer to right op
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	bc		;hi order word first
	push	de		;low order word second
	exx			;restore pointer to left op
	call	asfladd		;perform the addition
	pop	de		;pop original lo word
	pop	hl		;and high word
	ret			;and return with it in the right place

lfdec:
	exx
	ld	hl,mone		;get a minus one
	jp	incdec

	psect	data
; deff was a float-constant directive asz dropped; these are the 32-bit
; 1-sign/7-exp/24-mantissa encodings, bias 64: [mant-low, mant-mid, mant-high, sign+exp].
one:
	defb	00h, 00h, 80h, 41h	; 1.0
mone:
	defb	00h, 00h, 80h, 0C1h	; -1.0

; vim: tabstop=4 shiftwidth=4 noexpandtab:
