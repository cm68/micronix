;	Return value of the stack pointer

	psect	text
	global	__getsp
__getsp:
	ld	hl,0
	add	hl,sp
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
