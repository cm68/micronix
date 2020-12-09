#asm
	psect text
	global _errno
	global _close
_close:
	pop bc				; bc = retaddr
	pop hl				; hl = fd
	push hl				; restore stack size
	push bc
	defb 0cfH, 6
	ret nc
	ld (_errno),hl
	ld hl,-1
	ret
#endasm

