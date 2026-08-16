;	32-bit divide and remainder
;
;	Entry: HL':HL = dividend, DE':DE = divisor
;	Exit:  HL':HL = quotient or remainder
;	Clobbers: DE':DE, A, A'.  BC is saved and restored.
;
;	See QLONG.md for the convention.
;
;	The interesting part of this file is how little of it is new.
;	Hi-Tech's divide already worked on "dividend in HL/HL', divisor
;	in DE/DE', high words in the selected register set", which is
;	this convention with the shadow bank selected - so divide, sgndiv
;	and negif are its code, unchanged.  What went was lregset and
;	iregset, forty lines of shuffling whose whole job was getting a
;	long out of HLDE and a second one off the stack into exactly the
;	layout the arithmetic already wanted.  Here that is one exx.
;
;	The core is duplicated from ldiv.s rather than shared with it.
;	Sharing would mean a ccc-compiled program pulling ldiv.o for the
;	sake of divide and getting lregset, iregset and the whole
;	Hi-Tech entry surface with it; the linker works an object at a
;	time, and this tree has 64K to fit into.

	global	qdiv, qmod, qudiv, qumod, qneg

	psect	text

;	Signed divide.  The quotient is negative exactly when the signs
;	of the dividend and the divisor differ, so the sign is worked out
;	first, parked in A', and applied to the magnitude at the end.
qdiv:
	push	bc			;the caller's register variable
	exx				;select the high words
	ld	a,h
	xor	d			;bit 7 = the sign of the quotient
	ex	af,af'
	call	sgndiv			;quotient in bc/bc'
	call	qfrombc			;-> hl':hl, and back to the main bank
	ex	af,af'
	jp	m,qnegret
	pop	bc
	ret

;	Signed remainder.  The rule is that its sign is the sign of the
;	dividend.
qmod:
	push	bc
	exx
	ld	a,h			;the dividend's sign
	ex	af,af'
	call	sgndiv			;remainder left in hl/hl'
	exx				;-> main bank: hl':hl is the remainder
	ex	af,af'
	or	a
	jp	m,qnegret
	pop	bc
	ret

qudiv:
	push	bc
	exx
	call	divide
	call	qfrombc
	pop	bc
	ret

qumod:
	push	bc
	exx
	call	divide			;remainder is already in hl/hl'
	exx
	pop	bc
	ret

qnegret:
	call	qneg
	pop	bc
	ret

;	The quotient comes back in BC/BC' with the high word in the
;	selected set.  Move it to HL':HL, ending in the main bank.
qfrombc:
	ld	h,b
	ld	l,c			;high word
	exx
	ld	h,b
	ld	l,c			;low word
	ret

;	HL':HL = -HL':HL.  Clobbers DE':DE.
;
;	The borrow out of the low word has to reach the high one, and
;	nothing between them touches the flags: ex de,hl does not, ld
;	does not, and neither does exx.
qneg:
	ex	de,hl			;de = low word
	ld	hl,0
	or	a
	sbc	hl,de			;hl = 0 - low
	exx
	ex	de,hl			;de = high word
	ld	hl,0
	sbc	hl,de			;hl = 0 - high - borrow
	exx
	ret

;	Called with the high words selected; divides the absolute values,
;	so the quotient is positive.  Hi-Tech's, unchanged.

sgndiv:
	call	negif			;make dividend positive
	exx
	ex	de,hl			;put divisor in HL/HL'
	exx
	ex	de,hl
	call	negif			;make divisor positive
	ex	de,hl			;restore divisor to DE/DE'
	exx
	ex	de,hl
	exx				;select high words again
	jp	divide			;do division

negif:	;called with high word in HL, low word in HL'
	;returns with positive value

	bit	7,h			;check sign
	ret	z			;already positive
	exx				;select low word
	ld	c,l
	ld	b,h
	ld	hl,0
	or	a
	sbc	hl,bc
	exx
	ld	c,l
	ld	b,h
	ld	hl,0
	sbc	hl,bc
	ret				;finito

;	Called with dividend in HL/HL', divisor in DE/DE', high words in
;	selected register set
;	returns with quotient in BC/BC', remainder in HL/HL', high words
;	selected

divide:
	ld	bc,0			;initialize quotient
	ld	a,e			;check for zero divisor
	or	d
	exx
	ld	bc,0
	or	e
	or	d
	exx				;restor high words
	ret	z			;return with quotient == 0
	ld	a,1			;loop count
	jp	3f			;enter loop in middle
1:
	push	hl			;save divisor
	exx
	push	hl			;low word
	or	a			;clear carry
	sbc	hl,de			;subtract low word
	exx
	sbc	hl,de			;sbutract hi word
	exx
	pop	hl			;restore dividend
	exx
	pop	hl			;and hi word
	jr	c,2f			;finished - divisor is big enough
	exx
	inc	a			;increment count
	ex	de,hl			;put divisor in hl - still low word
	add	hl,hl			;shift left
	ex	de,hl			;put back in de
	exx				;get hi word
	ex	de,hl
	adc	hl,hl			;shift with carry
	ex	de,hl
3:
	bit	7,d			;test for max divisor
	jp	z,1b			;loop if msb not set

2:	;arrive here with shifted divisor, loop count in a, and low words
	;selected

3:
	push	hl			;save dividend
	exx
	push	hl			;low word
	or	a			;clear carry
	sbc	hl,de
	exx
	sbc	hl,de
	exx				;restore low word
	jp	nc,4f
	pop	hl			;restore low word of dividend
	exx
	pop	hl			;hi word
	exx				;restore low word
	jr	5f
4:
	inc	sp			;unjunk stack
	inc	sp
	inc	sp
	inc	sp
5:
	ccf				;complement carry bit
	rl	c			;shift in carry bit
	rl	b			;next byte
	exx				;hi word
	rl	c
	rl	b
	srl	d			;now shift divisor right
	rr	e
	exx				;get low word back
	rr	d
	rr	e
	exx				;select hi word again
	dec	a			;decrement loop count
	jr	nz,3b
	ret				;finished

; vim: tabstop=4 shiftwidth=4 noexpandtab:
