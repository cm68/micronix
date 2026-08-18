;	What this replaces.  The C is the reference: it says what
;	the routine means, and the assembly below says how this
;	machine does it.  Kept here rather than in a .c beside
;	this file, because a .c of the same name is a source the
;	makefile can pick up by accident, and did.
;
;	/*
;	 * el bizarro semantics: if we pass in a '\0', then hit at end.
;	 */
;	char *
;	index(s, c)
;	char *s;
;	char c;
;	{
;		do {
;			if (*s == c) return s;
;		} while (*s++);
;		return 0;	
;	}
;

	psect	text
	global	rcsv, rcret, _index

_index:	call	rcsv

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
