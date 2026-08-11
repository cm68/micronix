	psect	text
	global	_strcmp

;	bc is a register-variable home, and this used it to shuffle the
;	return address - which destroys the caller's copy before there
;	is any chance to save it.  So the save goes first, and the
;	arguments are read where they lie rather than popped.  Two bytes
;	here, on the stack, against two at every call site in the
;	compiler and a static that recursion would tread on.

_strcmp:
	push	bc		;the caller's register variable
	ld	hl,4
	add	hl,sp		;past the save and the return address
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		;de = first string
	inc	hl
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a		;hl = second string

1:	ld	a,(de)
	cp	(hl)
	jr	nz,2f
	inc	de
	inc	hl
	or	a
	jr	nz,1b
	ld	hl,0
	pop	bc
	ret

2:	ld	hl,1
	jr	nc,3f
	dec	hl
	dec	hl
3:	pop	bc
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
