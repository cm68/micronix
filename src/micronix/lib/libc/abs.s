;	abs(i) returns the absolute value of i

	global	_abs

	psect	text
_abs:
	bit	7,h		;Negative?  the argument arrives in hl
	ret	z		;no, leave alone
	ex	de,hl
	ld	hl,0
	or	a		;Clear carry
	sbc	hl,de
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
