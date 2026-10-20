;
; execve system call
;
; execve(name, argv, envp)
; char *name;
; char *argv[];
; char *envp[];
;
; Like exec, but the new image receives the environment vector
; instead of an empty one.  The kernel copies both vectors before
; the old image is released and lays out
;
;	argc, argv[0..], -1, envp[0..], 0
;
; at the top of the new image; crt0 points environ at envp.  A null
; envp, or one whose first word is null, gives an empty environment.
;
; returns only on error, with -1
;
	.extern _errno
	.global _execve

	.text
_execve:
	ld 	(name),hl		; the first argument arrives in hl
	pop 	hl			; discard ret addr
	pop 	hl			; second argument
	ld 	(args),hl
	pop 	hl			; third argument
	ld 	(env),hl

	ld 	hl,-6			; restore stack
	add 	hl,sp
	ld 	sp,hl

	rst 	08h
	.db 	000h
	.dw 	scall
	ld 	(_errno),hl		; execve only returns on error
	ld 	hl,-1
	ret

	.data
scall:	.db 	0cfh
	.db 	038h
name:	.dw 	0
args:	.dw 	0
env:	.dw 	0

; vim: set tabstop=8 shiftwidth=8 noexpandtab:
