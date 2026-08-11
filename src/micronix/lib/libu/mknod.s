;
; mknod system call
;
; mknod(name, mode, addr)
; char *name;
;
; Creates a new file. Unlike creat, it may be used to create
; directories and special files; it does not truncate or
; open files.
;
; The mode of the new file (including the file type bits) is
; taken from the mode argument, and the first address is
; taken from addr. For directories, this address should be
; 0, while for special files it should be the device number.
;
; This call is restricted to the super-user.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _mknod

	.text
_mknod:
	pop 	hl		; discard ret addr
	pop 	hl		; name
	ld 	(name),hl
	pop 	hl		; mode
	ld 	(mode),hl
	pop 	hl		; addr
	ld 	(addr),hl

	ld 	hl,-8		; restore stack
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
	.db 	00eh
name:	.dw 	0
mode:	.dw 	0
addr:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
