#asm
	psect text
	global _errno
	global _fork
_fork:
	defb	0cfH, 2			; system call jumps over next jump in child
	jp		1f

	ret		nc				; parent gets pid in hl
	ld		(_errno),hl
	ld		hl,-1
	ret

1:	ld		hl,0			; child get nada
	ret
#endasm

