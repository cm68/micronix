	global	_toupper

	psect	text
_toupper:
	ld	a,h		;check for a char - the argument is in hl
	or	a
	ret	nz
	ld	a,l
	cp	'a'
	ret	c		;Less than a
	cp	'z'+1
	ret	nc		;More than z
	sub	'a'-'A'
	ld	l,a
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
