	psect	text
	global	rcsv, rcret, _strchr
;
; strchr(s, c) - first c in s, or 0.  strchr is index under its ANSI
; name, and the match is tested before the terminator so that a '\0'
; matches the terminator itself rather than returning 0.
;
_strchr:
	call	rcsv

1:
	ld	a,(hl)
	cp	e
	jr	z,4f
	or	a
	jr	z,2f
	inc	hl
	jr	1b
4:	jp	rcret

2:	ld	hl,0
	jp	4b

; vim: tabstop=4 shiftwidth=4 noexpandtab:
