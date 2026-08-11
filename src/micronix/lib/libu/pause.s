;
; pause system call
;
; pause()
;
; Suspends the calling process until a signal is received.
;
; returns when signal received
;
	.global _pause

	.text
_pause:
	rst 	08h
	.db 	000h
	.dw 	scall
	ld 	hl,0
	ret

	.data
scall:	.db 	0cfh
	.db 	01dh

; vim: tabstop=8 shiftwidth=8 noexpandtab:
