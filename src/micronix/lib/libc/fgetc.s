;	/*
;	 *	fgetc for Zios stdio
;	 */
;
;	#include	<stdio.h>
;
;	#define	CPMEOF	032		/* ctrl-Z */
;
;	fgetc(f)
;	register FILE *	f;
;	{
;		int	c;
;
;		if(f->_flag & _IOEOF || !(f->_flag & _IOREAD)) {
;	reteof:
;			f->_flag |= _IOEOF;
;			return EOF;
;		}
;	loop:
;		if(f->_cnt > 0) {
;			c = (unsigned)*f->_ptr++;
;			f->_cnt--;
;		} else if(f->_flag & _IOSTRG)
;			goto reteof;
;		else
;			c = _filbuf(f);
;		if(f->_flag & _IOBINARY)
;			return c;
;		if(c == '\r')
;			goto loop;
;		if(c == CPMEOF) {
;			f->_cnt++;
;			f->_ptr--;
;			goto reteof;
;		}
;		return c;
;	}

;	The assembler version of the above routine
;
;	BC is the caller's register-variable home and IY the caller's
;	frame pointer; both are callee-saved.  The old body popped its
;	argument off the stack, so a save pushed at entry would have
;	come back as the argument - this one reads the argument where
;	it sits, past the saves and the return address, the way strcmp
;	does.  Everything saved is on the real stack, so setjmp and
;	longjmp owe this routine nothing.  fgets keeps its destination
;	in BC across this call, and used to lose it on the first
;	buffer refill.
;
;	Returns an int: the character with the high byte clear, or
;	EOF, which is -1 - a value no data byte can be.

ptr	equ	0
cnt	equ	2
base	equ	4
flag	equ	6
file	equ	7

IOREAD_BIT	equ	0	; _IOREAD is 01 - bit 0, not bit 1 (that is _IOWRT)
IOEOF_BIT	equ	4
IOBINARY_BIT	equ	7
IOSTRG_BIT	equ	6

RETURN	equ	0x0d
CPMEOF	equ	0x1a

	global	_fgetc, __filbuf
	psect	text

_fgetc:
	push	bc			;the caller's register variable
	push	iy			;and frame pointer; f rides here
	ld	hl,6
	add	hl,sp			;past the saves and the return
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	push	de
	pop	iy			;iy = f

	ld	a,(iy+flag)		;get flag bits
	bit	IOREAD_BIT,a
	jr	z,reteof		;return EOF if not open for read
	bit	IOEOF_BIT,a		;already seen EOF?
	jr	nz,reteof		;yes, repeat ourselves

loop:
	ld	l,(iy+cnt)
	ld	h,(iy+cnt+1)
	ld	a,l
	or	h			;any bytes left?
	jr	z,fill			;no, go get some more
	dec	hl
	ld	(iy+cnt),l		;update count
	ld	(iy+cnt+1),h
	ld	l,(iy+ptr)		;get the pointer
	ld	h,(iy+ptr+1)
	ld	a,(hl)
	inc	hl
	ld	(iy+ptr),l		;update pointer
	ld	(iy+ptr+1),h

got:
	bit	IOBINARY_BIT,(iy+flag)	;binary mode returns everything
	jr	nz,retch
	cp	RETURN			;text mode: \r vanishes
	jr	z,loop			;get another instead
	cp	CPMEOF			;and ctrl-Z is the end
	jr	nz,retch		;anything else goes back as is

	ld	a,(iy+base)		;buffered?
	or	(iy+base+1)
	jr	z,reteof		;no: nothing to put the ctrl-Z into
	ld	l,(iy+cnt)
	ld	h,(iy+cnt+1)
	inc	hl			;reset count
	ld	(iy+cnt),l
	ld	(iy+cnt+1),h
	ld	l,(iy+ptr)
	ld	h,(iy+ptr+1)
	dec	hl			;reset pointer
	ld	(iy+ptr),l
	ld	(iy+ptr+1),h

reteof:
	set	IOEOF_BIT,(iy+flag)	;note EOF
	ld	hl,-1			;EOF, which is -1 - not the ctrl-Z
	jr	done			;that marks it in a CP/M text file

retch:
	ld	l,a			;the character, high byte clear
	ld	h,0
	jr	done

fill:
	bit	IOSTRG_BIT,(iy+flag)	;end of string?
	jr	nz,reteof		;yes, return EOF
	push	iy			;pass f as the argument
	call	__filbuf		;refill; returns the char or EOF
	pop	de			;the argument off again
	bit	7,h
	jr	nz,reteof		;the refill met the end
	ld	a,l
	jr	got

done:
	pop	iy
	pop	bc
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
