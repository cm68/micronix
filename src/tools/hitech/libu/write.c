#asm
	psect text
	global _errno
	global _write
_write:
	pop bc				; bc = retaddr
	pop de				; de = fd
	pop hl				; hl = buffer
	ld (sys_wr+2),hl	
	pop hl				; hl = count
	ld (sys_wr+4),hl
	ex de,hl			; hl = fd	
	push bc				; restore stack size
	push bc
	push bc
	push bc
sys_wr:
	defb 0cfH, 4, 0, 0, 0, 0
	ret nc
	ld (_errno),hl
	ld hl,-1
	ret
#endasm

