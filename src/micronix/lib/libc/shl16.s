;	16-bit shift helpers for code generator

	psect	text
	global	shl16, shr16, ushr16

; HL = HL << DE (logical left shift)
; Shift count in E
shl16:
	ld	a,e
	or	a
	ret	z
1:	add	hl,hl
	dec	a
	jr	nz,1b
	ret

; HL = HL >> DE (arithmetic right shift, signed)
; Shift count in E
shr16:
	ld	a,e
	or	a
	ret	z
1:	sra	h
	rr	l
	dec	a
	jr	nz,1b
	ret

; HL = HL >> DE (logical right shift, unsigned)
; Shift count in E
ushr16:
	ld	a,e
	or	a
	ret	z
1:	srl	h
	rr	l
	dec	a
	jr	nz,1b
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
