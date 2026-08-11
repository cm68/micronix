;
; getuid system call
; getuid()
;
	.global _getuid

	.text
_getuid:
	rst 08h
	.db 018h
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
