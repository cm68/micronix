;
; set the break
;
	.globl	__break
	.extern	_errno

	.text
__break:
	pop	bc
	pop	hl
	push	hl
	push	bc
	ld	(addr),hl
	.db	0cfh, 00h
	.dw	scall
	ld	bc,0
	ret	nc
	dec	bc
	ld	(_errno),hl
	ret

	.data
scall:	.db	0cfh, 011h
addr:	.dw	0	

