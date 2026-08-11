;
; break system call
;
; _break(addr)
;
; Sets the system's idea of the lowest memory location not
; used by the program (called the "break") to addr.
;
; Locations >= break and < stack pointer are not in the
; address space. The system will refuse to set the break
; above the stack pointer.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global __break

	.text
__break:
	pop 	de		; ret addr
	pop 	hl		; addr
	ld 	(addr),hl
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
	.db 	011h
addr:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
