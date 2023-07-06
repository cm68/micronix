; Put string routine

.text
.globl puts
.extern putc
puts:
	ld	a,(hl)
	or	a
	ret	z
	call	putc
	inc	hl
	jp	puts

.bss
.globl buffer
	.defl byte[32] buffer
