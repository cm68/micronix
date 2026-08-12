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
IOWRT_BIT	equ	1	; _IOWRT is 02
IOEOF_BIT	equ	4
IOBINARY_BIT	equ	7
IOSTRG_BIT	equ	6


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
;
; A read-write stream comes back from fseek with neither direction
; bit set, because the seek is what makes the next operation free to
; be either one.  Refusing that here was wrong twice: it returned EOF
; without reading, and it set _IOEOF on the way out, so the stream
; stayed dead for every call after.
;
; v7 leaves the decision to _filbuf, and so do we now: only a stream
; that is positively write-only has nothing to read.  Neither bit set
; falls through, _filbuf takes _IOREAD for it, and the read happens.
;
; asz is what found it - it seeks back over the temp file it just
; assembled into and copies it into the object, and copied nothing.
;
	bit	IOREAD_BIT,a
	jr	nz,isread		;open for reading, carry on
	bit	IOWRT_BIT,a		;write-only has nothing to read
	jr	nz,reteof
isread:
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

;
; This is a Unix stdio.  A byte is a byte: there is no text mode, \r
; is data, and ctrl-Z is the character 0x1a and not the end of
; anything.  All of that came in from CP/M, where the directory only
; records a file's length in 128 byte records and the last one has to
; be terminated in its content - and it meant a read of any file
; holding a 0x1a stopped early unless the caller remembered "b".
;
; _IOBINARY is left alone: fopen still records it, and nothing here
; needs to ask.
;
got:
	jr	retch			;the byte, whatever it is

reteof:
	set	IOEOF_BIT,(iy+flag)	;note EOF
	ld	hl,-1			;EOF is -1, and it is the only EOF
	jr	done

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
