;
; mount system call
;
; mount(device, on, ronly)
; char *device, *on;
;
; Informs the system that the given block device contains a
; file system. Subsequent references to the file "on" will
; refer to the root directory of the new file system. The
; old contents of "on" are inaccessible until the device is
; unmounted.
;
; If ronly is non-zero, the system will not allow writing on
; the device.
;
; This call is restricted to the super-user.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _mount

	.text
_mount:
	ld 	(dev),hl		; the first argument arrives in hl
	pop 	hl		; discard ret addr
	pop 	hl		; second argument
	ld 	(dir),hl
	pop 	hl		; third argument
	ld 	(ronly),hl

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
	.db 	015h
dev:	.dw 	0
dir:	.dw 	0
ronly:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
