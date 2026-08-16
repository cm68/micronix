;	32-bit subtract: HL':HL -= DE':DE
;
;	See QLONG.md for the convention.  or a clears the carry so the
;	first sbc is a plain subtract; the borrow then crosses the exx
;	into the second.

	psect	text
	global	qsub

qsub:
	or	a		;clear the borrow
	sbc	hl,de		;low words
	exx
	sbc	hl,de		;high words, with the borrow
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
