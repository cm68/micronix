	global	_in, _out, _inp, _outp
	psect	text

_in:
_inp:
	push	bc		;the caller's register variable
	ld	c,l		;port address arrives in hl
	ld	b,h
	in	l,(c)		;read port
	ld	h,0		;zero extend it
	pop	bc
	ret

_out:
_outp:
	push	bc		;the caller's register variable
	ld	c,l		;port address arrives in hl
	ld	b,h
	ld	hl,4
	add	hl,sp		;past the save and the return address
	ld	e,(hl)		;data
	out	(c),e		;output the data
	ld	l,c		;return value in hl also
	ld	h,0
	pop	bc
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
