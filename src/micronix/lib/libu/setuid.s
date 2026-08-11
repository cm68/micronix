;
; setuid system call
;
; setuid(uid)
;
; Sets the real and effective user and group IDs. The group
; IDs are set to the high byte of the argument, and the user
; IDs are set to the low byte. Only the super-user is
; permitted to change the real IDs.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _setuid

	.text
_setuid:
	pop 	de		; ret addr
	pop 	hl		; uid
	push 	hl
	push 	de

	rst 	08h
	.db 	017h
	ex 	de,hl
	ld 	hl,0
	ret 	nc
	ld 	(_errno),de
	dec 	hl
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
