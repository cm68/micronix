	psect	text
	global	_strlen
_strlen:	ex	de,hl		;the string arrives in hl
	ld	hl,0

1:	ld	a,(de)
	or	a
	ret	z
	inc	hl
	inc	de
	jr	1b

; vim: tabstop=4 shiftwidth=4 noexpandtab:
