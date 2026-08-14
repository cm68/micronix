;
; exec system call
;
; exec(name, argv)
; char *name;
; char *argv[];
;
; Overlays the calling core image with the named file, then
; transfers to the beginning of the new core image. There
; can be no return from a successful exec: the calling core
; image is lost.
;
; Previously opened files remain open (so stdin and stdout
; are preserved), and ignored signals remain ignored.
; Caught signals are reset to their default behavior.
;
; returns only on error, with -1
;
	.extern _errno
	.global _exec

	.text
_exec:
	ld 	(name),hl		; the first argument arrives in hl
	pop 	hl		; discard ret addr
	pop 	hl		; second argument
	ld 	(argv),hl

	ld 	hl,-4		; restore stack
	add 	hl,sp
	ld 	sp,hl

	rst 	08h
	.db 	000h
	.dw 	scall
	ld 	(_errno),hl	; exec only returns on error
	ld 	hl,-1
	ret

	.data
scall:	.db 	0cfh
	.db 	00bh
name:	.dw 	0
argv:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
