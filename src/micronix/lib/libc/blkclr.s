;	blkclr(ptr, size)
;	char *	ptr; unsigned short size;

;	Fills memory with size null bytes

;	The old body counted down in primary BC, the caller's
;	register-variable home, and never saved it.  The walk lives in
;	the shadow set now - the same trick bmove uses - so the
;	registers a caller cares about come through untouched.  The
;	shadow set is free scratch by convention: _signal saves it
;	around a handler.

	psect	text
	global	_blkclr

_blkclr:
	pop	de	;return address - de is caller-scratch
	exx
	pop	hl	;pointer
	pop	bc	;count
	push	bc	;the argument slots go back for the
	push	hl	;caller's cleanup
	ld	e,0

1:
	ld	a,c	;check for finished
	or	b
	jr	z,2f
	ld	(hl),e
	inc	hl
	dec	bc
	jr	1b

2:
	exx
	push	de
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
