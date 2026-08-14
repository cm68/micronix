	global	rcsv, rcret

;	The prologue the hand-written string routines share: ix is the
;	frame pointer, iy and ix are saved, and the three arguments end
;	up in hl, de and bc - the first arrives in hl already and rides
;	through untouched, the other two are loaded from the stack.
;
;	That last one is why this saves bc as well.  Loading an argument
;	into it destroys the caller's register variable, so every one of
;	these routines clobbered BC and every call site in the compiler
;	paid two bytes to work around it.  The save costs one byte here
;	and shifts the argument offsets by two; the bodies read their
;	arguments out of registers and never touch the frame, so it is
;	contained.  They exit through rcret rather than cret, which is
;	shared with the csv convention above and cannot grow a pop.

ARG	equ	8		;offset of the 2nd arg: bc, ix, iy, return
	psect	text
rcsv:
	ex	(sp),iy		;save iy, get return address
	push	ix
	push	bc		;the caller's register variable
	ld	ix,0
	add	ix,sp		;new frame pointer
	ld	e,(ix+ARG+0)
	ld	d,(ix+ARG+1)
	ld	c,(ix+ARG+2)
	ld	b,(ix+ARG+3)
	jp	(iy)

rcret:
	ld	sp,ix
	pop	bc
	pop	ix
	pop	iy
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
