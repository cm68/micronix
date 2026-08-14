	psect	text
	global	_strcmp

;	bc is a register-variable home and nothing here touches it.  An
;	old body used it to shuffle the return address - destroying the
;	caller's copy - and the save that guarded against that outlived
;	the shuffle; both are gone.  The first string arrives in hl, the
;	second is read where it lies rather than popped.

_strcmp:
	ex	de,hl		;de = first string, which arrived in hl
	ld	hl,2
	add	hl,sp		;past the return address
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
	ret

2:	ld	hl,1
	ret	nc
	dec	hl
	dec	hl
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
