; Put char routine

	.text
	.globl putc
	.extern sys
putc:
	ld	b,1
	call	sys
	ret
