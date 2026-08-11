	psect	text
	global	_sbrk,__Hbss, _brk, _checksp

;	NB This brk() does not check that the argument is reasonable.
;
;	bc is a register-variable home.  These used it to shuffle the
;	return address and to hold the stack margin, which destroyed the
;	caller's copy - and the caller here is malloc, whose own
;	prologue saves its caller's bc but not the value it is holding
;	across this call.  Saved on the stack, after the arguments are
;	back the way they came in.

_brk:
	pop	hl	;return address
	pop	de	;argument
	ld	(memtop),de	;store it
	push	de		;adjust stack
	jp	(hl)	;return

_sbrk:
	pop	hl	;return address - not bc, see above
	pop	de
	push	de
	push	hl
	push	bc		;the caller's register variable
	ld	hl,(memtop)
	ld	a,l
	or	h
	jr	nz,1f
	ld	hl,__Hbss
	ld	(memtop),hl
1:
	add	hl,de
	jr	c,2f		;if overflow, no room
	ld	bc,1024		;allow 1k bytes stack overhead
	add	hl,bc
	jr	c,2f		;if overflow, no room
	sbc	hl,sp
	jr	c,3f
2:
	ld	hl,-1		;no room at the inn
	pop	bc
	ret

3:	ld	hl,(memtop)
	push	hl
	add	hl,de
	ld	(memtop),hl
	pop	hl
	pop	bc
	ret

_checksp:
	push	bc
	ld	hl,(memtop)
	ld	bc,128
	add	hl,bc
	sbc	hl,sp
	pop	bc
	ld	hl,1		;true if ok
	ret	c		;if carry, sp > memtop+128
	dec	hl		;make into 0
	ret

	psect	bss
memtop:	defs	2

; vim: tabstop=4 shiftwidth=4 noexpandtab:
