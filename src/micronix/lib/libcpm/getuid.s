;	Set/get uid for CP/M

	global	_getuid, _setuid, csv, cret

	entry	equ	5		;CP/M system call entry
	sguid	equ	20h		;set/get uid call value
	arg	equ	6		;offset of 1st arg

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
_getuid:
	call	csv
	push	bc
	ld	c,sguid			;get/set uid code
	ld	e,0FFh			;to get rather than set
	push	ix
	call	entry
	pop	ix
	ld	l,a
	ld	h,0
	pop	bc		;before cret drops the stack to ix
	jp	cret

_setuid:
	call	csv
	push	bc
	ld	e,(ix+arg)		;get argument
	ld	c,sguid
	push	ix
	call	entry
	pop	ix
	pop	bc		;before cret drops the stack to ix
	jp	cret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
