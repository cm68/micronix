	psect	text
	global	rcsv, rcret, _strchr

_strchr:
	call	rcsv

	jr	3f
1:
	inc	hl
3:
	ld	a,(hl)
	or	a
	jr	z,2f
	cp	e
	jr	nz,1b
4:	jp	rcret

2:	ld	hl,0
	jp	4b

; vim: tabstop=4 shiftwidth=4 noexpandtab:
