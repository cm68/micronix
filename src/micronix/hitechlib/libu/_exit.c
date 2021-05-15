/*
 * micronix hitech c
 * libu/_exit.c
 */
#asm
	psect text
	global _errno
	global __exit
__exit:
	pop bc				; bc = retaddr
	pop hl				; hl = exit code
	defb 0cfH, 1		; no point in restoring stack
#endasm

