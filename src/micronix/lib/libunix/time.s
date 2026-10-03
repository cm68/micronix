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
	; returns: hl:de = time (hl=high, de=low)
	pop 	bc		; bc = tp
	ld 	a,b
	or 	c
	jr 	z,9f		; tp is NULL
	; store 32-bit time to *tp, high word first - ccc's long keeps its
	; high half at the lower address (QLONG.md, NUXI), and hl is that
	; half, which is why this store is in register order and not
	; swapped: the two instructions below put hl's bytes down first.
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
; time returns a long, and ccc's long lives in HL':HL - high half in the
; shadow bank, low half in HL.  The kernel hands back hl:de, the other
; way round, so the two pairs have to cross: de's word becomes HL and
; hl's becomes HL'.
;
; This used to read the kernel's answer backwards - it took de for the
; high word and pushed it into the shadow bank, leaving hl (the true
; high word) in HL, and every long time() returned was swapped.  Nobody
; noticed because every caller in the tree reads the *tp store above
; instead of the return value, and the store, being written in register
; order, came out right.
;
	ex	de,hl		; hl = low word, de = high
	push	de
	exx
	pop	hl		; hl' = high word
	exx
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
