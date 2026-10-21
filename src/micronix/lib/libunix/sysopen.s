;
; open system call, raw
;
; sysopen(name, flags)
; char *name;
;
; The system call itself.  open() in open.c is what a program calls:
; it handles O_CREAT and O_TRUNC around this, and passes only the
; access mode and O_APPEND here.  The kernel masks the access mode.
;
; Opens the named file for reading (mode 0), writing
; (mode 1), or both (mode 2). The returned file descriptor
; should be saved for subsequent calls to read, write,
; and close.
;
; There is a limit of 16 open files per process.
;
; returns -1 on error, else file descriptor
;
	.extern _errno
	.global _sysopen

	.text
_sysopen:
	ld 	(path),hl		; the first argument arrives in hl
	pop 	hl		; discard ret addr
	pop 	hl		; second argument
	ld 	(mode),hl

	ld 	hl,-4		; restore stack
	add 	hl,sp
	ld 	sp,hl

	rst 	08h
	.db 	000h
	.dw 	scall
	ret 	nc		; fd in hl
err:	ld 	(_errno),hl
	ld 	hl,-1
	ret

	.data
scall:	.db 	0cfh
	.db 	005h
path:	.dw 	0
mode:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
