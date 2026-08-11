;	setjmp, longjmp - non local goto
;
;	What has to come back to life at the setjmp return is exactly
;	what the calling convention says a function keeps for its
;	caller: SP, the return address, IY (the frame pointer), IX and
;	BC (the register-variable homes).  Five words, so jmp_buf is
;	int[5]:
;
;		buf[0]	IY
;		buf[1]	IX
;		buf[2]	BC
;		buf[3]	return address
;		buf[4]	SP as the caller sees it after the return
;
;	The old body saved four and paid for it twice over: it parked
;	the return address in BC - the caller's register variable,
;	never restored - and longjmp came back with BC holding the
;	caller's old frame pointer.  Any function that kept a register
;	variable and called setjmp lost it on the spot, both ways in.
;
;	This one follows the library rule (see csv.s): nothing of the
;	caller's is popped, arguments are read where they sit, and
;	there is no state outside the registers and the caller's
;	jmp_buf - no statics, so a signal handler that longjmps out of
;	an interrupted longjmp abandons nothing that matters.
;
;	setjmp touches only A, DE, HL and the flags - the caller-
;	scratch set - so BC, IX and IY do not need restoring; they
;	were never disturbed.  longjmp scribbles freely: it never
;	returns, and everything the destination cares about is
;	reloaded from the buf.  The shadow BC ferries the saved BC
;	across the stack switch; the shadow set is library scratch by
;	convention, and _signal saves it around handlers.
;
;	longjmp(buf, 0) returns 1, as C requires - a setjmp that
;	answers 0 answered it from the call, always.

	psect	text
	global	_setjmp, _longjmp

_setjmp:
	ld	hl,2
	add	hl,sp
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		;de = buf
	ex	de,hl		;hl = cursor into buf, de = ferry

	push	iy
	pop	de
	ld	(hl),e
	inc	hl
	ld	(hl),d		;buf[0] = iy
	inc	hl
	push	ix
	pop	de
	ld	(hl),e
	inc	hl
	ld	(hl),d		;buf[1] = ix
	inc	hl
	ld	(hl),c
	inc	hl
	ld	(hl),b		;buf[2] = bc
	inc	hl

	ex	de,hl		;de = cursor
	ld	hl,0
	add	hl,sp		;hl -> the return address, in place
	ld	a,(hl)
	ld	(de),a
	inc	hl
	inc	de
	ld	a,(hl)
	ld	(de),a		;buf[3] = return address
	inc	hl		;hl = entry sp + 2: where the caller's
	inc	de		;sp stands after our ret pops
	ex	de,hl		;hl = cursor, de = that sp
	ld	(hl),e
	inc	hl
	ld	(hl),d		;buf[4] = sp

	ld	hl,0		;the answer that means "from the call"
	ret

_longjmp:
	ld	hl,2
	add	hl,sp
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		;de = buf
	inc	hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)		;bc = the value, parked where nothing
				;below needs to pass through
	ld	a,b
	or	c
	jr	nz,1f
	inc	bc		;longjmp(buf, 0) returns 1
1:
	ex	de,hl		;hl = cursor into buf
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	push	de
	pop	iy		;iy back (the old stack is still good
				;for the ferry)
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	push	de
	pop	ix		;ix back
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	push	de		;the saved BC crosses in the shadow bank:
	exx			;primary BC is carrying the value and
	pop	bc		;primary DE is about to carry the return
	exx			;address
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		;de = return address
	inc	hl
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a		;hl = the saved sp
	ld	sp,hl		;- and this is the moment of the jump

	push	de		;return address on the revived stack
	push	bc		;the value, for a breath
	exx
	push	bc		;the saved BC, out of the shadow bank
	exx
	pop	bc		;bc back
	pop	hl		;hl = the value
	ret			;"returns" from setjmp a second time

; vim: tabstop=8 shiftwidth=8 noexpandtab:
