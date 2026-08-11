;
; assembly source for exit system call
;
; /usr/src/lib/libu/exit.s
;
; Changed: <2023-07-07 01:18:37 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;
	.globl	_exit
	.extern	c.ret
	.extern	c.ent
	.extern	__exit
	
	.text
_exit:	call	c.ent
	ld	hl,0x4
	add	hl,de
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	bc
	call	__exit
	pop	af
	jp	c.ret
