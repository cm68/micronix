;	char bdos(func, arg)

	global	csv,cret

entry	equ	5		; CP/M entry point

arg	equ	8		;argument to call
func	equ	6		;desired function

	global	_bdos

	psect	text
;
;	BC is saved and given back.  csv and cret are HiTech's frame
;	helpers and save IY and IX only, but ccc keeps register
;	variables in BC and expects a callee to hand the caller's back
;	- that is what fentb and fexb do for compiled code.  A hand
;	written routine has to do it itself, and none of these did.
;	Every one of them loads C with the function number, and the
;	BDOS destroys BC on its own account, so a caller with a
;	register variable got it back as whatever the system left
;	there.  See the rule at the end of libc/csv.s.
;
_bdos:
	call	csv
	push	bc
	ld	e,(ix+arg)
	ld	d,(ix+arg+1)
	ld	c,(ix+func)
	push	ix
	push	iy
	call	entry
	pop	iy
	pop	ix
	ld	l,a
	rla
	sbc	a,a
	ld	h,a
	pop	bc		;before cret drops the stack to ix
	jp	cret
