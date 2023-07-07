;
; assembly source for chmod system call
;
; /usr/src/lib/libu/chmod.s
;
; Changed: <2023-07-07 01:00:08 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

	.extern	_errno
	.globl	_chmod

	.text
_chmod:	ld	hl, 0x2
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	ld	(name), hl
	ld	hl, 0x4
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	ld	(mode), hl
	.db	0xcf, 0x00
	.dw	sys
	ld	bc, 0x0
	ret	nc
	dec	bc
	ld	(_errno), hl
	ret

	.data
sys:	.db	0xcf, 	0x0f
name:	.dw	0
mode:	.dw	0
