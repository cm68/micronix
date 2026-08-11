;
; alarm system call
;
; alarm(seconds)
;
; Causes a SIGALRM (14) signal to occur after the specified
; number of seconds.
;
; returns the previous alarm value
;
	.global _alarm

	.text
_alarm:
	pop 	de		; ret addr
	pop 	hl		; seconds
	push 	hl
	push 	de

	rst 	08h
	.db 	000h
	.dw 	scall
	ret

	.data
scall:	.db 	0cfh
	.db 	01bh

; vim: tabstop=8 shiftwidth=8 noexpandtab:
