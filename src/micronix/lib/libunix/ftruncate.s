;
; ftruncate system call
;
; ftruncate(fd, length)
; int fd;
; long length;
;
; Truncates the open file to at most length bytes.  The file must be
; open for writing.  If the file was larger, the extra data is lost; a
; length at or past the current size is a no-op.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _ftruncate

	.text
_ftruncate:
	ld	a,l		; fd (the first argument) in a
	pop	hl		; discard return address
	pop	hl		; length high word
	ld	(length+2),hl
	pop	hl		; length low word
	ld	(length),hl

	ld	hl,-6		; restore the stack: ret + two words of length
	add	hl,sp
	ld	sp,hl

	ld	l,a		; fd in hl
	ld	h,0
	rst	08h
	.db	000h
	.dw	scall
	ex	de,hl		; save the kernel's answer (errno) in de
	ld	hl,0
	ret	nc		; success: return 0
	ld	(_errno),de
	dec	hl		; -1
	ret

	.data
scall:	.db	0cfh
	.db	02eh		; 46 = ftruncate
length:	.dw	0, 0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
