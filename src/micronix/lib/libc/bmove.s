;	bmove(from, to, count)

	global	_bmove, _movmem
	psect	text

_movmem:
_bmove:
	push	hl		;from, crossing to the shadow bank on the stack
	exx
	pop	hl		;from
	pop	af		;the return address, held across the pops
	pop	de		;to
	pop	bc		;count
	push	bc		;stack is as it was
	push	de
	push	af		;the return address back in place
	ld	a,b
	or	c
	jr	z,1f
	ldir
1:
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
