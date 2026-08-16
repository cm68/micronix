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

ptr	equ	0
cnt	equ	2
base	equ	4
flag	equ	6
file	equ	7

IOREAD_BIT	equ	1
IOEOF_BIT	equ	4
IOBINARY_BIT	equ	7
IOSTRG_BIT	equ	6

RETURN	equ	0x0d
CPMEOF	equ	0x1a

;	The value fgetc returns at end of file.  stdio.h spells it
;	(-1) for C; this file needs it as an assembler constant and
;	never had one, so "ld hl,EOF" below assembled to a reference
;	the linker could not resolve.  Nothing had linked this object
;	until CP/M did.
EOF	equ	-1

	global	_fgetc, __filbuf
	psect	text

_fgetc:
	pop	de			;get return address off stack
	ex	(sp),iy			;save iy and get arguement into iy
	ld	a,(iy+flag)		;get flag bits
	bit	IOREAD_BIT,a
	jr	z,reteof		;return EOF if not open for read
	bit	IOEOF_BIT,a		;Already seen EOF?
	jr	nz,reteof		;yes, repeat ourselves

loop:
	ld	l,(iy+cnt)
	ld	h,(iy+cnt+1)
	ld	a,l
	or	h			;any bytes left?
	jr	z,1f			;no, go get some more
	dec	hl
	ld	(iy+cnt),l		;update count
	ld	(iy+cnt+1),h
	ld	l,(iy+ptr)		;get the pointer
	ld	h,(iy+ptr+1)
	ld	a,(hl)
	inc	hl
	ld	(iy+ptr),l		;update pointer
	ld	(iy+ptr+1),h
2:
	bit	IOBINARY_BIT,(iy+flag)	;Binary mode?
	jr	z,3f			;no, check for EOF etc
retch:
	ld	l,a			;return the character in a
	ld	h,0
	ex	(sp),iy			;restore iy
	push	de			;put return address back
	ret				;with char in hl

3:
	cp	RETURN			;carriage return
	jr	z,loop			;yes, get another instead
	cp	CPMEOF			;end of file?
	jr	nz,retch		;no, return it!
	ld	a,(iy+base)		;buffered?
	or	(iy+base+1)
	jr	z,reteof		;yup, leave count alone
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
	ld	hl,EOF
	ex	(sp),iy			;restore iy
	push	de
	ret				;return with EOF in hl

1:
	bit	IOSTRG_BIT,(iy+flag)	;end of string?
	jr	nz,reteof		;yes, return EOF
	push	de			;save de
	push	iy			;pass iy as argument
	call	__filbuf		;refill the buffer
	ld	a,l			;the returned value
	pop	bc
	pop	de			;return address in de again
	bit	7,h
	jr	nz,reteof		;returned EOF
	jr	2b
