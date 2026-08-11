;
; access system call
;
; access(name, mode)
; char *name;
;
; Permission to access the named file, in the specified
; mode, is tested. The test is based on the real user and
; group IDs, rather than the effective IDs, so that a
; set-user-id program may test the permissions of its
; invoker. Mode is the sum of any of the following:
;
; 4  read
; 2  write
; 1  execute
;
; returns 0 if permitted, -1 if not
;
	.extern _errno
	.global _access

	.text
_access:
	pop 	hl		; discard ret addr
	pop 	hl		; name
	ld 	(name),hl
	pop 	hl		; mode
	ld 	(mode),hl

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
	.db 	021h
name:	.dw 	0
mode:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
