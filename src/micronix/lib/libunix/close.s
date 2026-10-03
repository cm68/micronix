;
; close system call
;
; close(fd)
;
; Given a file descriptor previously returned by open,
; creat, or pipe, close closes the associated file.
; A close of all files is automatic on exit, but since
; processes are limited to 16 simultaneously open files,
; close may be necessary for programs that deal with
; many files.
;
; passes fd in hl
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _close

	.text
_close:

	ld 	h,0		; fd in hl
	rst 	08h
	.db 	006h
	ex 	de,hl
	ld 	hl,0
	ret 	nc
	ld 	(_errno),de
	dec 	hl
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
