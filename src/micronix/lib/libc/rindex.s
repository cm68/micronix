;	What this replaces.  The C is the reference: it says what
;	the routine means, and the assembly below says how this
;	machine does it.  Kept here rather than in a .c beside
;	this file, because a .c of the same name is a source the
;	makefile can pick up by accident, and did.
;
;	/*
;	 * rightmost c in s
;	 */
;	char *
;	rindex(s, c)
;	char *s;
;	char c;
;	{
;		char *ret = 0;
;
;		do {
;			if (*s == c) ret = s;
;		} while (*s++);
;		return ret;	
;	}
;

	psect	text
	global	rcsv, rcret, _rindex

_rindex:
	call	rcsv

	ld	bc,0
	jr	5f
6:
	inc	hl
	inc	bc
5:
	ld	a,(hl)
	or	a
	jr	nz,6b
1:
	dec	hl
	ld	a,c
	or	b
	jr	z,2f
	dec	bc
	ld	a,(hl)
	cp	e
	jr	nz,1b
4:	jp	rcret

2:	ld	hl,0
	jp	4b

; vim: tabstop=4 shiftwidth=4 noexpandtab:
