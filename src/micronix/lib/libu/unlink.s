;
; unlink system call
;
; unlink(name)
; char *name;
;
; Removes the indicated entry from its directory. If this
; was the last link to the file, the file is removed and
; its space is freed. If the file was open in any process,
; this removal is delayed until the file is closed, even
; though its last directory entry has disappeared.
;
; In order to unlink a file, a user must have write
; permission on its directory. Write permission is not
; required on the file itself. Only the super-user can
; unlink a directory.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _unlink

	.text
_unlink:
	pop 	de		; ret addr
	pop 	hl		; name
	ld 	(name),hl
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
	.db 	00ah
name:	.dw 	0


; vim: tabstop=8 shiftwidth=8 noexpandtab:
