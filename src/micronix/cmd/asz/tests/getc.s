; Get char routine

.text
.globl getc
getc:
	ld	b,2
	call	sys
	ret
