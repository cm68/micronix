;
; assembly source for chdir system call
;
; /usr/src/lib/libu/chdir.s
;
; Changed: <2023-07-07 00:51:11 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;
	.globl	_chdir
    	.extern	_errno

	.text
_chdir: ld	hl, 0x2
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	ld	(name), hl
	.db	0xcf, 0x00
	.dw	sys
	ld	bc, 0x0
	ret	nc
	dec	bc
	ld	(_errno), hl
	ret

	.data
sys:	.db	0xcf, 0x00
name:	.dw	0
