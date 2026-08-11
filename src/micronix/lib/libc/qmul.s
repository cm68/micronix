;	32-bit multiply: HL':HL *= DE':DE
;
;	Entry: HL':HL = multiplicand, DE':DE = multiplier
;	Exit:  HL':HL = the low 32 bits of the product
;	Clobbers: DE':DE, A.  BC is saved and restored.
;
;	See QLONG.md for the convention.
;
;	Shift and add, thirty-two times.  The three 32-bit quantities and
;	the counter want seven registers' worth and there are six pairs,
;	so the counter is A - which is free because the multiplier bit is
;	tested in the carry, falling out of the shift that produces it,
;	and never needs a register of its own.
;
;	  product      HL':HL   where the answer has to end up anyway
;	  multiplicand DE':DE   doubled each turn
;	  multiplier   BC':BC   halved each turn, bit 0 into carry
;
;	BC' is free under this convention, which is what makes the third
;	pair available at all; BC is the caller's register variable and
;	is pushed for the duration.
;
;	Every exx in the body is matched by another before the branch, so
;	the loop is entered at the same parity every time.  Each pair of
;	them carries a carry across - out of the low word into the high
;	one for the add and the doubling, and out of the high word into
;	the low one for the halving, which runs the other way.

	psect	text
	global	qmul

qmul:
	push	bc		;the caller's register variable
	;
	; Rearrange: the multiplier to BC':BC, the multiplicand to DE':DE,
	; and nought into HL':HL to accumulate in.
	;
	ld	c,e
	ld	b,d		;bc  = multiplier low
	ex	de,hl		;de  = multiplicand low
	ld	hl,0		;product low
	exx
	ld	c,e
	ld	b,d		;bc' = multiplier high
	ex	de,hl		;de' = multiplicand high
	ld	hl,0		;product high
	exx
	ld	a,32
1:
	;
	; multiplier >>= 1, and the bit that falls out is the one to test
	;
	exx
	srl	b		;high word first, a zero in at the top
	rr	c
	exx
	rr	b		;low word, taking the bit from above
	rr	c		;and out comes multiplier bit 0
	jr	nc,2f
	add	hl,de		;product += multiplicand
	exx
	adc	hl,de
	exx
2:
	;
	; multiplicand <<= 1, low word first so the carry goes upwards
	;
	sla	e
	rl	d
	exx
	rl	e
	rl	d
	exx
	dec	a
	jr	nz,1b
	pop	bc		;the caller's back
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
