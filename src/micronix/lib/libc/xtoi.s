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

;	bc is the caller's register variable and this uses it to add
;	the digit in, so the save goes first, on the stack, where it
;	survives recursion and any depth of nesting.

_xtoi:	push	bc		;the caller's register variable
	ex	de,hl		;the string arrives in hl and walks in de
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

_ishex:	ld	a,l		;the character arrives in hl
	ld	hl,0
	call	hexdig
	ret	c
	inc	hl
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
