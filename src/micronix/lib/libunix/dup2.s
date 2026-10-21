;
; dup2 system call
;
; dup2(fd, fd2)
; int fd, fd2;
;
; Makes fd2 a synonym for fd.  If fd2 names an open file, that file is
; closed first.  Unlike dup, the descriptor is chosen by the caller, so
; moving a descriptor onto 0, 1 or 2 costs one call and cannot disturb
; any other descriptor.  dup2(fd, fd) does nothing and succeeds.
;
; returns fd2 on success, -1 with errno set on failure
;
	.extern _errno
	.global _dup2

	.text
_dup2:
	ld 	a,l		; fd (the first argument) in a
	pop 	hl		; discard return address
	pop 	hl		; fd2
	ld 	(fd2),hl

	ld 	hl,-4		; restore the stack: ret + one word
	add 	hl,sp
	ld 	sp,hl

	ld 	l,a		; fd in hl
	ld 	h,0
	rst 	08h
	.db 	000h
	.dw 	scall
	ret 	nc		; fd2 in hl
err:	ld 	(_errno),hl
	ld 	hl,-1
	ret

	.data
scall:	.db 	0cfh
	.db 	02dh		; 45 = dup2
fd2:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
