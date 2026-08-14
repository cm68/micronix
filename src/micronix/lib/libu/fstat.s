;
; fstat system call
;
; fstat(fd, buf)
; struct stat *buf;
;
; Gets the status of an open file (via the file descriptor)
; rather than on files given by name. This is often used to
; examine the status of stdin and stdout, whose names are
; usually unknown. Buf is the address of a 36 byte buffer.
; See stat for the structure format.
;
; passes fd in hl
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _fstat

	.text
_fstat:
	ld 	a,l		; fd arrives in hl
	pop 	hl		; discard ret addr
	pop 	hl		; buf
	ld 	(buf),hl

	ld	hl,-4
	add	hl,sp
	ld	sp,hl

	ld 	l,a		; fd in hl
	ld 	h,0
	rst 	08h
	.db 	000h
	.dw 	scall
	ex 	de,hl
	ld 	hl,0
	ret 	nc
	ld 	(_errno),de
	dec 	hl
	ret

	.data
scall:	.db 	0cfh
	.db 	01ch
buf:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
