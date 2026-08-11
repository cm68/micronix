;
; chown system call
;
; chown(name, owner)
; char *name;
;
; The owner of the file is changed to the low byte of
; "owner", and the group is changed to the high byte.
; Only the super-user may execute this call.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _chown

	.text
_chown:
	pop 	hl		; discard ret addr
	pop 	hl		; name
	ld 	(name),hl
	pop 	hl		; owner
	ld 	(owner),hl

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
	.db 	010h
name:	.dw 	0
owner:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
