;
; write system call
;
; write(fd, buf, nbytes)
; char *buf;
;
; Writes nbytes from the indicated buffer to the given
; open file. The number of bytes actually written is
; returned. Unlike read, this number should be the same
; as requested; otherwise, an error is indicated.
;
; passes fd in hl
;
; returns -1 on error, else nbytes written
;
	.extern _errno
	.extern fdadd
	.global _write

	.text
_write:
	ld 	a,l		; fd arrives in hl
	ld 	(fd),a		; save fd for fdadd
	pop 	hl		; discard ret addr
	pop 	hl		; buf
	ld 	(buf),hl
	pop 	hl		; nbytes
	ld 	(count),hl

	ld 	hl,-6		; restore stack
	add 	hl,sp
	ld 	sp,hl

	ld 	l,a		; fd in hl
	ld	h,0
	rst 	08h
	.db 	000h
	.dw 	scall
	jr 	c,err		; write count in hl
	ld 	a,(fd)
	jp 	fdadd		; advance _fdpos[fd], count in hl
err:	ld 	(_errno),hl
	ld 	hl,-1
	ret

	.data
scall:	.db 	0cfh
	.db 	004h
buf:	.dw 	0
count:	.dw 	0
fd:	.db 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
