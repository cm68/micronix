;	32-bit logical right shift: HL':HL >>= A, zeroes shifted in
;
;	Entry: HL':HL = the value, A = the count
;	Exit:  HL':HL shifted; DE':DE and BC untouched
;
;	srl where qsar.s has sra, and otherwise the same routine; see
;	there for the loop parity and qshl.s for the counter.

	psect	text
	global	qshr

qshr:
	or	a
	ret	z
	cp	33
	jr	c,1f
	ld	a,32
1:
	exx
	ld	b,a		;the count, in the free half
2:
	srl	h		;high word, a zero shifted in at the top
	rr	l
	exx			;-> main
	rr	h		;low word
	rr	l
	exx			;-> shadow
	djnz	2b
	exx			;-> main
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
