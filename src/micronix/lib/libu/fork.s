;
; assembly source for fork system call
;
; /usr/src/lib/libu/fork.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;
	.globl	_fork
	.extern	_errno

	.text
_fork:	.db	0xcf, 0x02
	jp	child
	ld	c, l
	ld	b, h
	ret	nc
	ld	bc, 0xffff
	ld	(_errno), hl
	ret
child:	ld	bc, 0x0
	ret
