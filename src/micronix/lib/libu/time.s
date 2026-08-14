;
; time system call
;
; time(tp)
; long *tp;
;
; Fills the long value pointed to by the argument with the
; number of seconds since 0:00 GMT January 1 1970.
;
; returns time, also stores to *tp if not NULL
;
	.global _time

	.text
_time:

	push	bc		; the caller's register variable: tp
				; lands in bc below and bc is a home
	push 	hl		; save tp
	rst 	08h
	.db 	00dh
	; returns: de:hl = time (de=high, hl=low)
	pop 	bc		; bc = tp
	ld 	a,b
	or 	c
	jr 	z,9f		; tp is NULL
	; store 32-bit time to *tp (little-endian)
	ld 	a,l
	ld 	(bc),a
	inc 	bc
	ld 	a,h
	ld 	(bc),a
	inc 	bc
	ld 	a,e
	ld 	(bc),a
	inc 	bc
	ld 	a,d
	ld 	(bc),a
9:	pop	bc
;
; time returns a long, and ccc's long lives in HL':HL - the kernel
; hands back de:hl (de high), so the high word crosses to the shadow
; bank before the return.  Every caller in the tree uses the *tp
; store instead, but the declared return type is owed its convention.
;
	push	de
	exx
	pop	hl		; hl' = high word
	exx
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
