; Part 1 of hello world test

.extern puts

.text
	ld	hl, hello_s
	call	puts
	ret
	
.data
.globl hello_s
	.def word hello_s
	.defl byte hello_s "Hello, world\n\0"
