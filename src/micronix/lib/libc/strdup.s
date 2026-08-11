;
; strdup - return a malloc'd copy of a string, 0 if out of memory
;
; Hand-written: the ccc-compiled version passed junk (caller's BC)
; as the malloc size.  BC is preserved (callee-saved register
; variables in both hitech and ccc).
;
	.extern	_malloc
	.global	_strdup

	.text
_strdup:
	push	bc		; callee-saved
	ld	hl,4
	add	hl,sp
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		; de = s
	push	de		; save s
	ld	hl,0
1:	ld	a,(de)		; hl = strlen(s) + 1 (count incl NUL)
	inc	de
	inc	hl
	or	a
	jr	nz,1b
	push	hl		; save count
	push	hl		; arg: malloc(count)
	call	_malloc
	pop	af		; clean arg
	pop	bc		; bc = count
	pop	de		; de = s
	ld	a,h
	or	l
	jr	z,2f		; no memory: return 0
	push	hl		; save dst (return value)
	ex	de,hl		; hl = s, de = dst
	ldir			; copy count bytes incl NUL
	pop	hl		; return dst
2:
	pop	bc
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
