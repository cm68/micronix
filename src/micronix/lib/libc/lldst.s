;	Long (32-bit) load/store helpers for code generator
;
;	A long lives in HLDE with the HIGH word in HL, which is what
;	ladd, lsub, lrelop and the rest of the runtime use and what the
;	compiler emits.  This file used to claim the opposite in its
;	header and lld used to end with an ex de,hl that made good on the
;	claim, so a value loaded through it came back with its halves
;	swapped.  Nothing called it, which is the only reason it never
;	showed.  lstde was right all along.

	psect	text
	global	lld, lldde, lstde, ldw, stide

; Load 32-bit from (HL) into HLDE
; Entry: HL = pointer to long
; Exit: HLDE = 32-bit value, HL = high word, DE = low word
lld:
	ld	e,(hl)		; low word from the lower address
	inc	hl
	ld	d,(hl)
	inc	hl
	ld	a,(hl)		; high word from the higher one
	inc	hl
	ld	h,(hl)
	ld	l,a
	ret

; Load 32-bit from (DE) into HLDE
; Entry: DE = pointer to long
; Exit: HLDE = 32-bit value
lldde:
	ex	de,hl
	call	lld
	ret

; Store HLDE to (dest), dest pointer on the stack
; Entry: HLDE = 32-bit value, stack has dest pointer
; Exit: value stored, pointer popped
;
; BC comes through ALIVE.  It is the register-variable home, and the
; old body parked the return address there: every long store made by
; a function keeping a pointer in BC - tokcpy, with the destination
; token - handed that pointer back as a text address, and the fields
; stored after the long went through it into whatever it now named.
; That was cpp's intern pool, six-byte token strides at a time.  The
; return address sits in a scratch word instead; this code has no
; reentrancy to lose.
lstde:
	ex	(sp),hl		; hl = return address, TOS = high word
	ld	(lsret),hl
	pop	hl		; high word back
	ex	(sp),hl		; hl = dest ptr, TOS = high word
	ld	(hl),e
	inc	hl
	ld	(hl),d
	inc	hl
	pop	de		; high word
	ld	(hl),e
	inc	hl
	ld	(hl),d
	ld	hl,(lsret)
	jp	(hl)

; Load 16-bit from (HL) into HL
; Entry: HL = pointer
; Exit: HL = 16-bit value
;
; Called ldw, not ldi: ldi is a Z80 instruction, so "ldi:" was
; assembled as one and the label was never defined.  The global stayed
; undefined and went out in the object that way, which nothing noticed
; until the compiler first called lld and pulled this object into a
; link.  Nothing calls this one yet; it is right so that it can be.
ldw:
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	ret

; Store DE to (HL) and advance
; Entry: HL = pointer, DE = value
; Exit: value stored, HL advanced by 2
stide:
	ld	(hl),e
	inc	hl
	ld	(hl),d
	inc	hl
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:

	.data
lsret:	.dw	0
