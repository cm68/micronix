;
; stty system call
;
; stty(fd, vec)
; struct sgtty *vec;
;
; Sets the status of the terminal associated with the file
; descriptor from the 6-byte structure pointed at by vec.
; See gtty for the structure format.
;
; passes fd in hl
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _stty

	.text
_stty:
	ld 	a,l		; fd arrives in hl
	pop 	de		; ret addr
	pop 	hl		; vec
	ld 	(buf),hl
	push 	hl		; the slot back for the caller's cleanup
	push 	de

	ld 	h,0
	ld 	l,a
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
	.db 	01fh
buf:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
