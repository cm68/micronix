;
; assembly source for _exit system call
;
; /usr/src/lib/libu/_exit.s
;
; Changed: <2023-07-07 01:13:08 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;

_exit.o:
	.globl	__exit

	.text
__exit:	pop	bc
	pop	hl
	push	hl
	push	bc
	.db	0xcf, 0x01
