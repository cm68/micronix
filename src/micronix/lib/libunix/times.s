;
; times system call
;
; times(tp)
; struct tms *tp;
;
; Fills the struct tms pointed to by tp with the process times.  See
; sys/times.h for the structure.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _times

	.text
_times:
	ld 	(tp),hl		; the first argument arrives in hl
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
	.db 	02bh
tp:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
