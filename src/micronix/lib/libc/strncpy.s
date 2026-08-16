	psect	text
	global	_strncpy, rcsv, rcret

_strncpy:
	call	rcsv
	push	hl

1:
	ld	a,c
	or	b
	jr	z,2f
	dec	bc
	ld	a,(de)
	ld	(hl),a
	inc	hl
	or	a
	jr	z,1b
	inc	de
	jr	1b

2:	pop	hl
	jp	rcret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
