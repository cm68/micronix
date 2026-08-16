;	long relational	operation - returns flags as though
;	a long subtract	was done.
;
;	arelop is the signed comparison and returns the answer in the
;	sign flag; lrelop is the unsigned one and returns it in carry,
;	the same places the 16-bit code looks for them.  The two used to
;	be the same routine, so every unsigned long comparison was being
;	answered as a signed one: 0xffffffff came out below 1 rather
;	than above it.
;
;	Both take the left operand in HLDE, high word in HL, and the
;	right on the stack with its high word pushed first, and both
;	leave the stack clean.

	psect	text
	global	lrelop,arelop

arelop:
	exx			;select	alternate reg set
	pop	hl		;return	address
	exx			;get other set again
	pop	bc		;low word of 2nd arg
	ex	de,hl		;put hi	word of	1st in de
	ex	(sp),hl		;get hi	word of	2nd in hl
	ex	de,hl		;hi word of 1st	back in	hl
	ld	a,h		;test for differing signs
	xor	d
	jp	p,2f		;the same, so ok
	ld	a,h		;get the sign of the LHS
	or	1		;ensure zero flag is reset, set sign flag
	pop	hl		;unjunk stack
	jp	1f		;return	with sign of LHS
2:
	or	a
	sbc	hl,de		;set the flags
	pop	hl		;low word of 1st into hl again
	jr	nz,1f		;go return if not zero
	sbc	hl,bc		;now set flags on basis	of low word
	jr	z,1f		;if zero, all ok
	ld	a,2		;make non-zero
	rra			;rotate	carry into sign
	or	a		;set minus flag
	rlca			;put carry flag	back

1:
	exx			;get return address
	jp	(hl)		;and return with stack clean

;	Unsigned: a plain 32-bit subtract, low word first so the borrow
;	carries into the high word.  Carry then means below.
;
;	Zero is the awkward half.  The subtraction leaves Z set from the
;	high word alone, so the low word has to be folded in - and or
;	clears carry, which is the other half of the answer.  So keep a
;	copy of the borrow in a register, do the or chain, and rotate the
;	copy back into carry: rla touches carry and leaves Z alone, and
;	ld does not touch flags at all.

lrelop:
	exx			;select	alternate reg set
	pop	hl		;return	address
	exx			;get other set again
	pop	bc		;low word of 2nd arg
	ex	de,hl		;lo word of 1st	in hl
	or	a		;clear carry for the first subtract
	sbc	hl,bc		;low difference, borrow	out
	ex	de,hl		;hi word of 1st	back in	hl, lo diff in de
	pop	bc		;hi word of 2nd	arg
	sbc	hl,bc		;hi difference,	carry =	unsigned below
	sbc	a,a		;ff if borrow, 00 if not - and carry survives
	ld	b,a		;keep it
	ld	a,h		;now find out whether all four bytes
	or	l		;of the	difference were zero
	or	d
	or	e		;Z set if equal, carry lost
	ld	a,b		;ld does not touch the flags
	rla			;carry back from bit 7,	Z untouched
	exx			;get return address
	jp	(hl)		;and return with stack clean

; vim: tabstop=4 shiftwidth=4 noexpandtab:
