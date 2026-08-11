;
; stat system call
;
; stat(name, buf)
; char *name;
; struct stat *buf;
;
; Gets the status of a named file. Buf is the address of a
; 36 byte buffer into which the status is placed. See fstat
; for the structure format.
;
; It is not necessary to have read permission on the file,
; but all directories leading to the file must be searchable.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _stat

	.text
_stat:
	pop 	hl		; discard ret addr
	pop 	hl		; name
	ld 	(name),hl
	pop 	hl		; buf
	ld 	(buf),hl

	ld 	hl,-6		; restore stack
	add 	hl,sp
	ld 	sp,hl

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
	.db 	012h
name:	.dw 	0
buf:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
