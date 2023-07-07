;
; assembly source for access system call
;
; /usr/src/lib/libu/access.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;
	.globl	_access
	.extern _errno

	.text
_access:	
	ld	hl, 0x2
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h,(hl)
	ld	l, a
	ld	(name), hl
	ld	hl, 0x4
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l,a
	ld	(mode), hl
	.db	0xcf, 0
	.dw	sys
	ld	bc, 0x0
	ret	nc
	dec	bc
	ld	(_errno), hl
	ret

	.data
sys:	.db	0xcf, 0x21
name:	.dw	0
mode:	.dw	0
