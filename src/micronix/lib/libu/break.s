;
; assembly source for break system call
;
; /usr/src/lib/libu/break.s
;
; Changed: <2023-07-07 00:50:24 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;
	.globl	__break
	.extern	_errno

	.text
__break:
	pop	bc
	pop	hl
	push	hl
	push	bc
	ld	(brk), hl
	.db	0xcf, 0x00
	.dw	sys
	ld	bc, 0x0
	ret	nc
	dec	bc
	ld	(_errno), hl
	ret

	.data
sys:	.db	0xcf, 0x11
brk:	.dw	0
