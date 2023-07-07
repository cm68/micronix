;
; assembly source for gtty system call
;
; /usr/src/lib/libu/gtty.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

	.globl	_gtty
	.extern	_errno

	.text
_gtty:	ld	hl, 0x4
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	ld	(vec), hl
	ld	hl, 0x2
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	.db	0xcf, 0x00
	.dw	sys
	ld	bc, 0x0
	ret	nc
	dec	bc
	ld	(_errno), hl
	ret

	.data
sys:	.db	0xcf, 0x20
vec:	.dw	0
