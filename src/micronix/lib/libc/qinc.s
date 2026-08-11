;	Step a long in memory, and hand back the value that was there
;
;	Entry: HL = address of the long
;	Exit:  HL':HL = the ORIGINAL value; memory stepped
;	Clobbers: DE, A.  BC and DE' come through untouched.
;
;	See QLONG.md for the convention.  Postfix is what these are for;
;	a prefix that wants its value reads it back afterwards, which is
;	what pass2 emits.
;
;	The Hi-Tech pair pushed the value and the address around the
;	stack through a gval/sval pair because there was nowhere else to
;	put four words.  There is now: the original high word goes
;	straight to HL' and stays there, so only the original low word
;	needs the stack.
;
;	inc bc and dec bc set no flags, so the carry between the halves
;	is found by testing the low word for the wrap - after an
;	increment it is zero, before a decrement it was.

	psect	text
	global	qinc, qdec

qinc:
	push	bc		;the caller's register variable
	ld	c,(hl)
	inc	hl
	ld	b,(hl)		;bc = low word
	inc	hl
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		;de = high word; hl -> address+3
	push	de
	exx
	pop	hl		;hl' = the original high word
	exx
	push	bc		;the original low word
	inc	bc
	ld	a,b
	or	c		;wrapped to zero?
	jr	nz,1f
	inc	de		;carry into the high word
1:
	jr	qstep

qdec:
	push	bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	inc	hl
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	push	de
	exx
	pop	hl		;hl' = the original high word
	exx
	push	bc		;the original low word
	ld	a,b
	or	c		;was it zero before the decrement?
	dec	bc
	jr	nz,qstep
	dec	de		;borrow out of the high word

;	Write the stepped value back.  HL is at the top of it, so this
;	walks down: high word first, low word last.
qstep:
	ld	(hl),d
	dec	hl
	ld	(hl),e
	dec	hl
	ld	(hl),b
	dec	hl
	ld	(hl),c
	pop	hl		;the original low word
	pop	bc		;and the caller's bc
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
