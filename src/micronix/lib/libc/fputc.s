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

;
;	NOT open for write is not the same as "cannot write".  fseek
;	leaves a read-write stream with neither direction bit set, and
;	the first operation afterwards is what decides which it is -
;	_filbuf takes _IOREAD when a read comes, _flsbuf takes _IOWRT
;	when a write does.  Returning EOF here meant the write never
;	reached _flsbuf and the stream was never decided, so nothing
;	could be written to a "w+" file after seeking in it.
;
;	So hand it to _flsbuf, which decides: it takes the stream if it
;	is undecided, and refuses with _IOERR if it is genuinely a
;	read-only one.  _cnt is 0 after a seek in any case, which is
;	the same path a full buffer takes.
;
	bit	IOWRT_BIT,(iy+flag)	;open for write?
	jr	z,flush
;
; This is a Unix stdio, so \n is a byte and goes out on its own.  The
; \r that used to be written ahead of it - and the ctrl-Z that fclose
; put on the end - are CP/M's, and a file written here is read back
; here: putting them in meant every byte count was wrong by the
; number of lines, and reading such a file needed "b" to get the \r
; back out again.
;
; _IOBINARY is still recorded by fopen; nothing here asks about it.
;
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

; vim: tabstop=8 shiftwidth=8 noexpandtab:
