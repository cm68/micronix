;	fputc for stdio - this version in assembler for speed

;	/*
;	 *	fputc for Zios stdio
;	 */
;
;	#include	<stdio.h>
;
;	fputc(c, f)
;	register FILE *	f;
;	uchar	c;
;	{
;		if(!(f->_flag & _IOWRT))
;			return EOF;
;		if((f->_flag & _IOBINARY) == 0 && c == '\n')
;			fputc('\r', f);
;		if(f->_cnt > 0) {
;			f->_cnt--;
;			*f->_ptr++ = c;
;		} else
;			return _flsbuf(c, f);
;		return c;
;	}

;	BC is the caller's register-variable home and IY the caller's
;	frame pointer; both are callee-saved.  The old body popped the
;	character straight into BC, so every call destroyed the
;	caller's register variable - fputs walks its string in BC and
;	wrote one character per call.  This one saves both on the real
;	stack and reads its arguments where they sit, the way strcmp
;	does, so setjmp and longjmp owe it nothing.  The \r recursion
;	below needs no care at all now: fputc preserves BC, and fputc
;	is its own caller.
;
;	Returns an int: the character written, or EOF, which is -1.

ptr     equ     0
cnt     equ     2
base    equ     4
flag    equ     6
file    equ     7

IOWRT_BIT       equ     1       ; _IOWRT is 02 - bit 1, not bit 2 (that is _IONBF)
IOBINARY_BIT    equ     7
IOSTRG_BIT      equ     6

NEWLINE equ     0x0a
RETURN  equ     0x0d
CPMEOF  equ     0x1a


	global	_fputc, __flsbuf
	psect	text

_fputc:
	push	bc			;the caller's register variable
	push	iy			;and frame pointer; f rides here
	ld	hl,6
	add	hl,sp			;past the saves and the return
	ld	c,(hl)			;c = the character
	ld	b,0			;with the top byte clear
	inc	hl
	inc	hl
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	push	de
	pop	iy			;iy = f

	bit	IOWRT_BIT,(iy+flag)	;open for write?
	jr	z,reteof
	bit	IOBINARY_BIT,(iy+flag)	;binary mode writes it as it is
	jr	nz,put
	ld	a,c			;text mode: a \r goes out ahead
	cp	NEWLINE			;of every \n
	jr	nz,put
	push	iy			;the file argument
	ld	hl,RETURN
	push	hl			;the character argument
	call	_fputc			;preserves BC, so c survives this
	pop	hl			;the arguments off again
	pop	hl

put:
	ld	l,(iy+cnt)
	ld	h,(iy+cnt+1)
	ld	a,l			;check count
	or	h
	jr	z,flush			;no room at the inn
	dec	hl			;update count
	ld	(iy+cnt),l
	ld	(iy+cnt+1),h
	ld	l,(iy+ptr)
	ld	h,(iy+ptr+1)		;get pointer
	ld	(hl),c			;store character
	inc	hl			;bump pointer
	ld	(iy+ptr),l
	ld	(iy+ptr+1),h
	ld	l,c			;return the character
	ld	h,b

done:
	pop	iy
	pop	bc
	ret

flush:
	push	iy			;the file argument
	push	bc			;the character argument
	call	__flsbuf		;returns the character or EOF
	pop	de			;the arguments off again
	pop	de
	jr	done

reteof:
	ld	hl,-1
	jr	done

; vim: tabstop=8 shiftwidth=8 noexpandtab:
