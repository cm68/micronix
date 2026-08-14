;
; creat system call
;
; creat(name, mode)
; char *name;
;
; If the file does not exist, and if the parent directory
; is writable, it is created with the given mode. If the
; file does exist and is writable, it is truncated to 0
; length, and its mode and owner remain unchanged. In
; either case, the file is opened for writing only.
;
; See chmod(2) for the construction of modes. Note that the
; file is opened for writing, even if the given mode does
; not allow writing.
;
; returns -1 on error, else file descriptor
;
	.extern _errno
	.extern fdclr
	.global _creat

	.text
_creat:
	ld 	(name),hl		; the first argument arrives in hl
	pop 	hl		; discard ret addr
	pop 	hl		; second argument
	ld 	(mode),hl

	ld 	hl,-4		; restore stack
	add 	hl,sp
	ld 	sp,hl

	rst 	08h
	.db 	000h
	.dw 	scall
	jr 	c,err		; fd in hl
	ld 	a,l
	jp 	fdclr		; zero _fdpos[fd], fd in hl
err:	ld 	(_errno),hl
	ld 	hl,-1
	ret

	.data
scall:	.db 	0cfh
	.db 	008h
name:	.dw 	0
mode:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
