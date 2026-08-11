;
; stime system call
;
; stime(tp)
; long *tp;
;
; Sets the system's idea of the date and time. The argument
; is a pointer to a long containing the number of seconds
; since 0:00 GMT, January 1, 1970.
;
; Only the super-user may make this call.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _stime

	.text
_stime:
	pop 	de		; ret addr
	pop 	hl		; tp pointer
	push 	hl
	push 	de

	; load 32-bit time from *tp (little-endian)
	ld 	e,(hl)
	inc 	hl
	ld 	d,(hl)
	inc 	hl
	ld 	a,(hl)
	inc 	hl
	ld 	h,(hl)
	ld 	l,a
	ex 	de,hl		; de:hl = time (de=high, hl=low)
	rst 	08h
	.db 	019h
	ex 	de,hl
	ld 	hl,0
	ret 	nc
	ld 	(_errno),de
	dec 	hl
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
