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

	psect	text
	global	tramp

tramp:
	jp	(hl)

; vim: tabstop=4 shiftwidth=4 noexpandtab:
