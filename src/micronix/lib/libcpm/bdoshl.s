;	Bdos calls which return values in HL

;	short	bdoshl(fun, arg);

	psect	text

entry	equ	5		; CP/M entry point

arg	equ	8		;argument to call
func	equ	6		;desired function


	global	_bdoshl
	global	csv,cret
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
_bdoshl:
	call	csv
	push	bc
	ld	e,(ix+arg)
	ld	d,(ix+arg+1)
	ld	c,(ix+func)
	push	ix
	call	entry
	pop	ix
	pop	bc		;before cret drops the stack to ix
	jp	cret		;return value already in hl
