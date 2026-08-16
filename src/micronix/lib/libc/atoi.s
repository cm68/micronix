;	What this replaces.  The C is the reference: it says what
;	the routine means, and the assembly below says how this
;	machine does it.  Kept here rather than in a .c beside
;	this file, because a .c of the same name is a source the
;	makefile can pick up by accident, and did.
;
;	atoi(p)
;	register char *p;
;	{
;		register int n;
;		register int f;
;
;		n = 0;
;		f = 0;
;		for(;;p++) {
;			switch(*p) {
;			case ' ':
;			case '\t':
;				continue;
;			case '-':
;				f++;
;			case '+':
;				p++;
;			}
;			break;
;		}
;		while(*p >= '0' && *p <= '9')
;			n = n*10 + *p++ - '0';
;		return(f? -n: n);
;	}
;

	psect	text
digit:	sub	'0'
	ret	c
	cp	10
	ccf
	ret

	global	_atoi
_atoi:	push	bc		;bc is the caller's register variable and
	ex	de,hl		;this uses it as a multiply scratch below.
	ld	hl,0		;the string arrives in hl and walks in de
1:
	ld	a,(de)
	inc	de
	cp	' '
	jr	z,1b
	cp	'	'	;tab
	jr	z,1b
	dec	de		;point to 1st non blank char
	cp	'-'
	jr	z,3f
	cp	'+'
	jr	nz,2f
	or	a		;reset zero flag
3:
	inc	de
2:	ex	af,af'
1:
	ld	a,(de)
	inc	de
	call	digit
	jr	c,3f
	add	hl,hl
	ld	c,l
	ld	b,h
	add	hl,hl
	add	hl,hl
	add	hl,bc
	ld	c,a
	ld	b,0
	add	hl,bc
	jr	1b

3:
	ex	af,af'
	jr	nz,4f
	ex	de,hl
	ld	hl,0
	sbc	hl,de
4:
	pop	bc		;the caller's, back
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
