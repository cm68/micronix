;
; getpid system call
; getpid()
;
	.global _getpid

	.text
_getpid:
	rst 08h
	.db 014h
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
