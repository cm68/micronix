;	32-bit arithmetic right shift: HL':HL >>= A, sign preserved
;
;	Entry: HL':HL = the value, A = the count
;	Exit:  HL':HL shifted; DE':DE and BC untouched
;
;	See qshl.s for why the count is in A and the loop counts in B',
;	and QLONG.md for the convention.  A right shift starts at the
;	top, so this one begins in the shadow bank where the high word
;	is and crosses to the main bank halfway through each turn.

	psect	text
	global	qsar

qsar:
	or	a
	ret	z
	cp	33
	jr	c,1f
	ld	a,32
1:
	exx
	ld	b,a		;the count, in the free half
2:
	sra	h		;high word, keeping the sign
	rr	l
	exx			;-> main
	rr	h		;low word, taking the bit shifted out
	rr	l
	exx			;-> shadow
	djnz	2b
	exx			;-> main
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
