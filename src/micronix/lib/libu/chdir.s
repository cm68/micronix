;
; chdir system call
;
; chdir(dirname)
; char *dirname;
;
; The working directory of the current process is changed
; to the given directory. The user must have search
; (execute) permission on the directory.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _chdir

	.text
_chdir:
	pop 	de		; ret addr
	pop 	hl		; dirname
	ld 	(path),hl
	push 	hl
	push 	de

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
	.db 	00ch
path:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
