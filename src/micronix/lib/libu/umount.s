;
; umount system call
;
; umount(device)
; char *device;
;
; Tells the system that the given special file should no
; longer be treated as a file system. The file on which the
; device was mounted reverts to its ordinary interpretation.
;
; Umount will return an error if there are still any active
; files on the mounted system.
;
; This call is restricted to the super-user.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _umount

	.text
_umount:
	pop 	de		; ret addr
	pop 	hl		; device
	ld 	(dev),hl
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
	.db 	016h
dev:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
