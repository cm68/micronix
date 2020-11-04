/
/ the whitesmith's c does not have this, but instead has
/ a somewhat cockamamie enter/leave and when/raise thing
/
/ setjmp is a much simpler mechanism
/
/ a jmp_buf is all the registers that define a
/ context.  on the z80, that's not much, since
/ the setjmp call saved all the important stuff on the
/ stack already.
/
/ typedef unsigned int jmp_buf[3]
/ 		sp
/ 		de (frame ptr)
/ 		retaddr
/ 
/ setjmp(jmp_buf env) { }
/
public _setjmp

_setjmp:

	bc <= sp			/ pop return address into bc
	hl <= sp			/ hl = &jmp_buf[0]
	hl => sp 			/ restore our stack
	bc => sp
	de => sp			/ save de

	hl <> de			/ de = &jmp_buf[0]
	hl = 0 + sp			/ hl = sp

	hl <> de			/ hl = jmp_buf[0], de = sp	
	de ->^ hl			/ jmp_buf[0] = sp
	hl + 1				/ hl = &jmp_buf[1]

	de <= sp			/ restore de
	de ->^ hl			/ jmp_buf[1] = de
	hl + 1				/ hl = &jmp_buf[2]

	bc ->^ hl			/ retaddr -> jmp_buf[2]

	bc = 0
	ret
/
/ we don't return from this function however, we do restore our
/ machine state to the previous condition and then return the arg
/
/ longjmp(jmp_buf, val) { }
/
public _longjmp
_longjmp:

	bc <= sp			/ throw away longjmp return addr

	hl <= sp			/ hl = &jmp_buf[0]
	bc <= sp			/ bc = return value from longjmp(j, v)

	de =^ hl			/ de = saved_sp
	hl + 1				/ hl = &jmp_buf[1]

	hl <> de			/ hl = saved sp, de = &jmp_buf[1]

	sp = hl				/ restored sp

	hl <> de			/ hl = &jmp_buf[1]
	de =^ hl			/ hl = saved de
	de => sp			/ save de to stack
	hl + 1
	de =^ hl			/ de = jmp_buf[2]
	hl = de				/ hl = retaddr
	de <= sp			/ restore de
	hl => sp			/ retaddr to stack

	ret
