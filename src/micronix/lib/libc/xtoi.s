	global	_xtoi, _ishex
	psect	text
hexdig:	cp	'0'
	ret	c
	cp	'9'+1
	jr	nc,1f
	sub	'0'
	ret

1:	cp	'A'
	ret	c
	cp	'F'+1
	jr	nc,2f
	sub	'A'-0Ah
	ret

2:	cp	'a'
	ret	c
	cp	'f'+1
	ccf
	ret	c
	sub	'a'-0ah
	ret

;	The return address comes back through hl, not bc: bc is the
;	caller's register variable and this uses it to add the digit in,
;	so it has to be saved - and it cannot be saved after the shuffle
;	has already overwritten it.  The save goes on the stack once the
;	arguments are back the way they came in, so it costs two bytes
;	and survives recursion and any depth of nesting.

_xtoi:	pop	hl	;return address
	pop	de
	push	de
	push	hl
	push	bc
	ld	hl,0
1:
	ld	a,(de)
	inc	de
	call	hexdig
	jr	c,9f
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	ld	c,a
	ld	b,0
	add	hl,bc
	jr	1b
9:	pop	bc		;the caller's, back
	ret

_ishex:	pop	hl
	pop	de
	push	de
	push	hl
	ld	a,e
	ld	hl,0
	call	hexdig
	ret	c
	inc	hl
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
