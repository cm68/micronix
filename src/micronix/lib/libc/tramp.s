;	Indirect call trampoline.
;
;	The Z80 can call an address it is given at assembly time and it
;	can jump to the one in HL, but it cannot call that one.  The
;	whole difference is the return address, so borrow one: a call to
;	here pushes it, the jump hands over to the function, and the
;	function's own ret comes back to the original caller.  This
;	routine never returns and is never on the stack.
;
;	Entry: HL = address to call, arguments already pushed.

;	With arguments, HL carries the first one and cannot carry the
;	address too, so the address arrives in DE - dead at every call -
;	and the borrowed return address goes on the stack before it:
;	push the target, ret into it, and the function's own ret still
;	comes back to the original caller.
;
;	Entry: DE = address to call, first argument in HL (HL':HL when
;	long), the rest already pushed.

	psect	text
	global	tramp, trampde

tramp:
	jp	(hl)

trampde:
	push	de
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
