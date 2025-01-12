; Part 1 of hello world test

	.extern puts

.text
	ld	hl, hello_p
	call	puts
	ret
	
	.data
	.globl hello_p, hello_s
hello_p:	
	.dw	hello_s
hello_s:
	.db	"Hello, world\n\0"
