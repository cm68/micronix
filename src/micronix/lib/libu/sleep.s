;
; sleep system call
;
; sleep(seconds)
;
; The calling process is suspended for at least the given
; number of seconds.
;
; returns 0 on success, -1 on early return from signal
;
	.global _sleep

	.text
_sleep:
	pop 	de		; ret addr
	pop 	hl		; seconds
	push 	hl
	push 	de

	rst 	08h
	.db 	023h
	ld 	hl,0
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
