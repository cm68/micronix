;	What this replaces.  The C is the reference: it says what
;	the routine means, and the assembly below says how this
;	machine does it.  Kept here rather than in a .c beside
;	this file, because a .c of the same name is a source the
;	makefile can pick up by accident, and did.
;
;	char *
;	strncmp(s1, s2, n)
;	char *s1;
;	char *s2;
;	int n;
;	{
;		while (--n >= 0 && *s1 == *s2++)
;			if (*s1++ == '\0')
;				return(0);
;		return(n<0 ? 0 : *s1 - *--s2);
;	}
;

	psect	text
	global	_strncmp, rcsv, rcret
;
; strncmp(char *s1, char *s2, int n)
;

_strncmp:
	call	rcsv

1:	ld	a,c
	or	b
	jp	z,3f
	dec	bc
	ld	a,(de)
	cp	(hl)
	jr	nz,2f
	inc	de
	inc	hl
	or	a
	jr	nz,1b
3:
	ld	hl,0
	jp	rcret

2:	ld	hl,1
	jp	c,rcret
	dec	hl
	dec	hl
	jp	rcret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
