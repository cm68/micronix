;	32-bit left shift: HL':HL <<= A
;
;	Entry: HL':HL = the value, A = the count
;	Exit:  HL':HL shifted; DE':DE and BC untouched
;
;	See QLONG.md for the convention.  Two things differ from the
;	Hi-Tech allsh besides the register layout.
;
;	The count arrives in A rather than B, and the loop counts in B' -
;	which is free, where B is the caller's register variable.  That
;	is why the rules calling this carry no BC save: there is nothing
;	left for the helper to tread on.
;
;	The loop is entered in the SHADOW bank so that djnz is reached at
;	the same parity every time round.  Get that wrong and the second
;	turn shifts the halves the other way about.  exx touches no flags
;	and djnz touches none either, so the carry out of the low word
;	crosses both into the adc.

	psect	text
	global	qshl

qshl:
	or	a		;a shift by nought is the value itself
	ret	z
	cp	33
	jr	c,1f		;anything past 32 bits is 32 bits
	ld	a,32
1:
	exx
	ld	b,a		;the count, in the free half
2:
	exx			;-> main
	add	hl,hl		;low word
	exx			;-> shadow
	adc	hl,hl		;high word, taking the carry
	djnz	2b
	exx			;-> main
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
