;	32-bit compare: HL':HL against DE':DE
;
;	See QLONG.md for the convention.
;
;	qcmp is the signed comparison and returns the answer in the sign
;	flag; qucmp is the unsigned one and returns it in carry, the same
;	places the 16-bit code looks for them.  Both set Z when the two
;	are equal.  The Hi-Tech pair these replace were once the same
;	routine, so every unsigned long comparison was answered as a
;	signed one and 0xffffffff came out below 1 rather than above it;
;	keeping them apart is the point of having two.
;
;	Both operands are in registers, so neither touches the stack and
;	both are a plain ret.  BC comes through alive; B' and C' are used
;	as scratch, which is free here.
;
;	A comparison consumes its operands - the answer is a flag - so
;	HL':HL and DE':DE are both left holding the difference.

	psect	text
	global	qcmp, qucmp

;	Signed.  A straight 32-bit subtract gives the wrong sign when the
;	operands' signs differ and the difference overflows, so that case
;	is settled by the left operand's sign alone and never subtracts.
qcmp:
	exx			;hl = high left, de = high right
	ld	a,h
	xor	d		;do the signs differ?
	jp	m,2f		;yes: a subtraction would overflow
	or	a		;no: clear the borrow
	sbc	hl,de		;high difference
	jr	nz,1f		;the high word settles it
	exx			;equal so far, so the low words decide
	or	a
	sbc	hl,de
	ret	z		;equal: Z set, sign clear, which is right
	;
	; The high words matched, so the order is the UNSIGNED order of
	; the low words and the answer is the borrow.  Move it into the
	; sign flag, which is where a signed comparison is read from.
	;
	ld	a,2
	rra			;carry into bit 7; a stays non-zero
	or	a		;sign = the borrow, Z clear
	ret
1:
	exx			;the sign of the high difference is the answer
	ret
2:
	ld	a,h		;the left operand's sign is the answer
	or	1		;keep bit 7, force non-zero so Z is clear
	exx
	ret

;	Unsigned: a plain 32-bit subtract, low word first so the borrow
;	carries into the high word.  Carry then means below.
;
;	Zero is the awkward half.  The subtraction leaves Z set from the
;	high word alone, so all four bytes have to be folded in - and or
;	clears carry, which is the other half of the answer.  So keep a
;	copy of the borrow, do the or chain, and rotate the copy back
;	into carry: rla touches carry and leaves Z alone, and ld does not
;	touch the flags at all.
qucmp:
	or	a		;clear the borrow
	sbc	hl,de		;low difference
	exx
	sbc	hl,de		;high difference, carry = unsigned below
	sbc	a,a		;ff if borrow, 00 if not - and carry survives
	ld	b,a		;keep it in b'
	ld	a,h		;fold the high difference
	or	l
	ld	c,a		;keep that in c'
	exx
	ld	a,h		;and the low difference
	or	l
	exx
	or	c		;Z now set only if all four bytes were zero
	ld	a,b		;ld does not touch the flags
	rla			;carry back from bit 7, Z untouched
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
