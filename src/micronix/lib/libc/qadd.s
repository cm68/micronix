;	32-bit add: HL':HL += DE':DE
;
;	See QLONG.md for the convention.  One exx brings both high words
;	into place at once and does not touch the flags, so the carry out
;	of the low word crosses it into the adc.
;
;	Six bytes, against fifteen for the Hi-Tech ladd it replaces - and
;	three at the call site against seven, because there is nothing to
;	push and no BC to protect.

	psect	text
	global	qadd

qadd:
	add	hl,de		;low words
	exx
	adc	hl,de		;high words, with the carry
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
