;	Functions to implement bitfields:
;
;	The width and offset (in bits) of the bitfield concerned appear in
;	the code immediately after the call, packed into one byte:
;
;		bit	|7    4|3   0|
;			|offset|width|
;
;	The instruction these replace is the undocumented SLL (CB 30..37),
;	which shifts a ONE into bit 0, not a zero.  Every mask came out
;	with its low bit set for free - width 3 gave 14 instead of 7, so
;	"x.b = 5" read back 4.  SLA is the shift-left with a zero.
;
;	bfext: called with the word in HL, returns the field in HL.  A is
;	scratch and the offset/width byte is read inline; BC is not touched
;	at all and DE is saved, so nothing the caller keeps there is lost.
;
;	bfins: called with the address in HL and the value in DE.  It
;	stores the field and returns the value in HL.  BC is saved in the
;	helper; the caller's BC survives.

	psect	text
	global	bfext, bfins
bfext:
	ex	(sp),hl		;hl = return address, (sp) = the address
	ld	a,(hl)		;pick up width and offset
	inc	hl
	ex	(sp),hl		;hl = address, return address past the byte
	push	de		;don't touch de
	push	af		;save the byte
	ld	a,(hl)		;the word the field lives in
	inc	hl
	ld	h,(hl)
	ld	l,a
	pop	af		;the byte again
	push	af		;and once more for the offset
	and	0Fh		;get width
	ld	de,1		;start with a 1
	inc	a
	dec	a
	jr	z,2f
1:
	sla	e
	rl	d		;shift up one bit
	dec	a
	jr	nz,1b
2:
	dec	de		;decrement by one
	pop	af		;the byte again
	rra
	rra
	rra
	rra
	and	0Fh		;get offset
	jr	z,4f		;skip if zero
3:
	srl	h
	rr	l
	dec	a
	jr	nz,3b
4:
	ld	a,l
	and	e		;mask appropriately
	ld	l,a
	ld	a,h
	and	d
	ld	h,a		;all done!
	pop	de
	ret

;	Insert the data in de into the data at (hl). Size and offset are in
;	the following byte as usual.  The caller's BC is saved here, so the
;	call site needs no push/pop of its own.

bfins:
	ex	(sp),hl		;hl = return address, (sp) = the address
	ld	a,(hl)		;get the info byte
	inc	hl
	ex	(sp),hl		;hl = address, return past the byte
	push	bc		;the caller's BC
	push	hl		;save the address
	push	af		;save the info byte
	ld	hl,1
	and	0Fh		;width
	jr	z,2f
	ld	b,a
1:
	sla	l
	rl	h		;shift up one bit
	djnz	1b
2:
	dec	hl		;convert to a mask
	ld	a,e
	and	l
	ld	e,a
	ld	a,d
	and	h
	ld	d,a		;masked it: de is the value to return
	pop	af		;the info byte again
	push	de		;save the value
	rra
	rra
	rra
	rra
	and	0Fh		;mask out offset
	jr	z,4f
	ld	b,a
3:
	sla	l		;shift the mask up
	rl	h
	sla	e		;and the value with it
	rl	d
	djnz	3b
4:
	ex	(sp),hl		;save the mask, get the value
	pop	bc		;mask in bc
	ex	(sp),hl		;address in hl, save the value
	ld	a,c		;complement the mask
	cpl
	and	(hl)		;and with the memory data
	or	e		;or with the shifted value
	ld	(hl),a		;put back
	inc	hl
	ld	a,b
	cpl
	and	(hl)
	or	d
	ld	(hl),a
	pop	hl		;get the value back
	pop	bc		;the caller's BC
	ret		;finito!

; vim: set tabstop=4 shiftwidth=4 noexpandtab:
