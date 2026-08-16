;	What this replaces.  The C is the reference: it says what
;	the routine means, and the assembly below says how this
;	machine does it.  Kept here rather than in a .c beside
;	this file, because a .c of the same name is a source the
;	makefile can pick up by accident, and did.
;
;	/*
;	 * copy n bytes from s to d
;	 *
;	 */
;
;	memcpy(d, s, n)
;	register char *	d, * s;
;	register int	n;
;	{
;		while(n--)
;			*d++ = *s++;
;	}
;

;	memcpy(d, s, n)
;
;	A byte at a time, through pointers the compiler reloaded on every
;	pass: a nine byte token cost several hundred cycles that way, and
;	tokcpy does it two hundred and twenty thousand times over one
;	source.
;
;	ldir is the instruction for this.  The shape below is bmove.s's,
;	which has been doing it correctly all along: cross to the shadow
;	bank and the count in bc costs nothing to save, because the bc
;	the caller wants back is in the other set.
;
;	ldir with bc zero copies sixty five thousand five hundred and
;	thirty six bytes, so the zero case is tested and not assumed.
;
;	hl is not touched by any of this - the push is only to get d
;	across the bank switch - so the destination is still there at the
;	ret, and this returns it, which is what memcpy is defined to do
;	and what the C version never did.

	global	_memcpy
	psect	text

_memcpy:
	push	hl		;d, crossing to the shadow bank on the stack
	exx
	pop	de		;d, the destination ldir wants
	pop	af		;the return address, held across the pops
	pop	hl		;s
	pop	bc		;n
	push	bc		;stack is as it was
	push	hl
	push	af
	ld	a,b
	or	c
	jr	z,1f
	ldir
1:
	exx
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
