;stdio.h: 6: extern	struct	_iobuf {
;stdio.h: 7: 	char *		_ptr;
;stdio.h: 8: 	int		_cnt;
;stdio.h: 9: 	char *		_base;
;stdio.h: 10: 	unsigned char		_flag;
;stdio.h: 11: 	char		_file;
;stdio.h: 12: } _iob[	6];
;stdio.h: 14: extern unsigned char _setup;
;stdio.h: 43: 	struct _iobuf *		fopen();
;stdio.h: 44: 	struct _iobuf *		freopen();
;stdio.h: 45: 	struct _iobuf *		fdopen();
;stdio.h: 46: long		ftell();
;stdio.h: 47: char *		fgets();
;stdio.h: 48: char *		_bufallo();
;P:SCANF.C: 7: extern int	_doscan();
;P:SCANF.C: 9: scanf(fmt, args)
;P:SCANF.C: 10: char *	fmt;
;P:SCANF.C: 11: int	args;
;P:SCANF.C: 12: {
psect	text
global	_scanf
_scanf:
global	ncsv, cret, indir
call	ncsv
defw	f8
;P:SCANF.C: 13: 	return _doscan(	(&_iob[0]), fmt, &args);
global	__doscan
global	__iob
push	ix
pop	de
ld	hl,8
add	hl,de
push	hl
ld	l,(ix+6)
ld	h,(ix+1+6)
push	hl
ld	hl,__iob
push	hl
call	__doscan
exx
ld	hl,2+2+2
add	hl,sp
ld	sp,hl
exx
ld	l,l
ld	h,h
jp	l2
;P:SCANF.C: 14: }
l2:
jp	cret
f8	equ	0

;P:SCANF.C: 9: scanf(fmt, args)
;P:SCANF.C: 10: char *	fmt;
;P:SCANF.C: 11: int	args;
;P:SCANF.C: 12: {
psect	te