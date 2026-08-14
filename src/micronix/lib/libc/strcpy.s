	psect	text
	global	_strcpy

;	bc holds the destination across the copy so it can be handed
;	back, and the return-address shuffle used it too.  Both destroy
;	the caller's register variable, so the save is the first thing
;	that happens and the arguments are read where they lie.

_strcpy:
	push	bc		;the caller's register variable
	ex	de,hl		;de = destination, which arrived in hl
	ld	hl,4
	add	hl,sp		;past the save and the return address
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a		;hl = source
	ld	c,e
	ld	b,d		;remember the destination

1:	ld	a,(hl)
	ld	(de),a
	inc	de
	inc	hl
	or	a
	jr	nz,1b
	ld	l,c
	ld	h,b		;the destination is the return value
	pop	bc
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
