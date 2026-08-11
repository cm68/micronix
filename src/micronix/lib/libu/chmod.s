;
; chmod system call
;
; chmod(path, mode)
;
; The  mode  of  the  file  is set as indicated. Only the
; owner of a file, or  the  super-user,  may  change  the
; mode.  Modes  are  constructed  by  ORing together some
; combination of the following (octal) values:
;
; 4000  Set user id on execution
; 2000  Set group id on execution
; 1000  Currently ignored
; 0400  Read by owner
; 0200  Write by owner
; 0100  Execute (or search directory) by owner
; 0070  Read, write, execute (search) by group
; 0007  Read, write, execute (search) by others
;
; returns -1 and sets errno if error, else returns 0
;
	.extern _errno
	.global _chmod

	.text
_chmod:
	pop 	hl		; discard ret addr
	pop 	hl		; path
	ld 	(path),hl
	pop 	hl		; mode
	ld 	(mode),hl

	ld 	hl,-6		; restore stack
	add 	hl,sp
	ld 	sp,hl

	rst 	08h
	.db 	000h
	.dw 	scall
	ex	de,hl
	ld 	hl,0
	ret 	nc
	ld 	(_errno),de
	dec	hl
	ret

	.data
scall:	.db 	0cfh
	.db 	00fh
path:	.dw 	0
mode:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
