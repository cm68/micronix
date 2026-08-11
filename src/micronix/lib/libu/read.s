;
; read system call
;
; read(fd, buffer, nbytes)
; char buffer[];
;
; A file descriptor is a word returned from a successful
; open, creat, dup, or pipe call. Buffer is a memory
; location where at most nbytes of data will be placed.
; The number of bytes actually read is returned. This may
; be less than nbytes; a read on a terminal will return
; at most one line. If 0 is returned, file is exhausted.
;
; passes fd in hl
;
; returns -1 on error, 0 on EOF, else bytes read
;
	.extern _errno
	.extern fdadd
	.global _read

	.text
_read:
	pop 	hl		; discard ret addr
	pop 	de		; fd in e
	ld 	a,e
	ld 	(fd),a		; save fd for fdadd
	pop 	hl		; buffer
	ld 	(buf),hl
	pop 	hl		; nbytes
	ld 	(count),hl

	ld 	hl,-8		; restore stack
	add 	hl,sp
	ld 	sp,hl

	ld 	l,e		; fd in hl
	ld 	h,0
	rst 	08h
	.db 	000h
	.dw 	scall
	jr 	c,err		; read count in hl
	ld 	a,(fd)
	jp 	fdadd		; advance _fdpos[fd], count in hl
err:	ld 	(_errno),hl
	ld 	hl,-1
	ret

	.data
scall:	.db 	0cfh
	.db 	003h
buf:	.dw 	0
count:	.dw 	0
fd:	.db 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
