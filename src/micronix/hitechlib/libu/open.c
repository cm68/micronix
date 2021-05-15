/*
 * micronix hitech c
 * libu/open.c
 */
#asm
	psect text
	global _errno
	global _open
_open:
	pop bc				; bc = retaddr
	pop hl				; hl = mode
	ld (sys_op+4),hl	
	pop hl				; hl = name
	ld (sys_op+2),hl
	push bc				; restore stack size
	push bc
	push bc
sys_op:
	defb 0cfH, 5, 0, 0, 0, 0
	ret nc
	ld (_errno),hl
	ld hl,-1
	ret
#endasm

