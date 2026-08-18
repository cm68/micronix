;	What this replaces.  The C is the reference: it says what
;	the routine means, and the assembly below says how this
;	machine does it.  Kept here rather than in a .c beside
;	this file, because a .c of the same name is a source the
;	makefile can pick up by accident, and did.
;
;	char *
;	strdup(s)
;	{
;	#ifdef LIBDEBUG
;		char xx[100];
;	#endif
;		char *r;
;		int n = strlen(s);
;
;		r = malloc(n + 1);
;		if (r) {
;			strcpy(r, s);
;		}
;	#ifdef LIBDEBUG
;		sprintf(xx, "strdup %s ret %s", s, r);
;		logmsg(xx);	
;	#endif
;		return r;	
;	}
;

;
; strdup - return a malloc'd copy of a string, 0 if out of memory
;
; Hand-written: the ccc-compiled version passed junk (caller's BC)
; as the malloc size.  BC is preserved (callee-saved register
; variables in both hitech and ccc).
;
; The inline count below is deliberate, and cpir is not.  strdup
; wants n+1 as a value - the malloc size and the ldir count - not
; strlen's n, and the loop counts n+1 straight into hl with nothing
; to convert.  cpir would hand the length over free but inverted:
; hl one past the NUL and bc holding -count, and turning that back
; into a positive count costs a 5-byte negate plus a push to keep
; the end pointer.  Every cpir shape assembles a byte larger (30
; against 29) for a faster scan (21 against ~35 cycles a byte) that
; nobody sees next to the malloc it feeds.
;
	.extern	_malloc
	.global	_strdup

	.text
_strdup:
	push	bc		; callee-saved
	push	hl		; save s, which arrived in hl
	ex	de,hl		; de walks it
	ld	hl,0
1:	ld	a,(de)		; hl = strlen(s) + 1 (count incl NUL)
	inc	de
	inc	hl
	or	a
	jr	nz,1b
	push	hl		; save count
	call	_malloc		; malloc(count): the count rides in hl
	pop	bc		; bc = count
	pop	de		; de = s
	ld	a,h
	or	l
	jr	z,2f		; no memory: return 0
	push	hl		; save dst (return value)
	ex	de,hl		; hl = s, de = dst
	ldir			; copy count bytes incl NUL
	pop	hl		; return dst
2:
	pop	bc
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
