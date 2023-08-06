.globl	_sub1
_sub1:
	mov	2(sp),r0
	sub	4(sp),2(r0)
	sbc	(r0)
	rts	pc
