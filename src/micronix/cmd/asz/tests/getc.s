; Get char routine

	.globl getc
	.text
getc:
	ld	b,2
	call	sys
	ret
