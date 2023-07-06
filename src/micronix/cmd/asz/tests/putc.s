; Put char routine

.text
.globl putc
putc:
	ld	b,1
	call	sys
	ret
