	psect	text
	global	_strncmp, rcsv, rcret
;
; strncmp(char *s1, char *s2, int n)
;

_strncmp:
	call	rcsv

1:	ld	a,c
	or	b
	jp	z,3f
	dec	bc
	ld	a,(de)
	cp	(hl)
	jr	nz,2f
	inc	de
	inc	hl
	or	a
	jr	nz,1b
3:
	ld	hl,0
	jp	rcret

2:	ld	hl,1
	jp	c,rcret
	dec	hl
	dec	hl
	jp	rcret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
