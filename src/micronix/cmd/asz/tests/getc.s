; Get char routine

	.globl getc
	.extern sys

	.text
getc:
	ld	b,2
	call	sys
	ret
