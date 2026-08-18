	psect	text
	global	rcsv, rcret, _strrchr
;
; strrchr(s, c) - last c in s, or 0.  strrchr is rindex under its
; ANSI name, and the terminator is matched like any other character,
; so strrchr(s, '\0') returns the terminator rather than 0.
;
_strrchr:
	call	rcsv

	ld	bc,0
	jr	5f
6:
	inc	hl
	inc	bc
5:
	ld	a,(hl)
	or	a
	jr	nz,6b
	cp	e
	jr	z,4f
1:
	dec	hl
	ld	a,c
	or	b
	jr	z,2f
	dec	bc
	ld	a,(hl)
	cp	e
	jr	nz,1b
4:	jp	rcret

2:	ld	hl,0
	jp	4b

; vim: tabstop=4 shiftwidth=4 noexpandtab:
