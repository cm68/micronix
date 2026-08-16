;
; brk/sbrk - user-space break tracking for CP/M
;
; libcpm's sibling of lib/libu/sbrk.s, minus the system call.  CP/M
; has no __break: a program owns the whole TPA and manages the break
; itself, so there is no kernel to refuse a store past the stack.
; memtop holds the current break, lazily started at the end of bss
; (__Hbss).  sbrk(n) grows the break by n and returns the OLD break
; (base of the granted region); sbrk(0) returns the current break.
; malloc depends on both, and stdio's buf.c calls sbrk(BUFSIZ) the
; first time a FILE is buffered.
;
; The stack lives at the top of the TPA, under the BDOS, so the guard
; against the heap growing up through it is the only protection there
; is - the one that, under micronix, __break did for us.
;
; BC is preserved: hitech and ccc keep register variables there.
;
	.extern __Hbss
	.global _brk, _sbrk, _memtop

	.text
_brk:
	ld	(_memtop),hl	; addr arrived in hl
	ld	hl,0		; success
	ret

_sbrk:
	push	bc		; callee-saved (C register variables)
	ex	de,hl		; de = increment, which arrived in hl
	ld	hl,(_memtop)
	ld	a,h
	or	l
	jr	nz,1f
	ld	hl,__Hbss	; first use: break starts past bss
	ld	(_memtop),hl
1:
	ld	a,d
	or	e
	jr	z,3f		; sbrk(0): return current break
	push	hl		; save old break
	add	hl,de		; hl = new break
	bit	7,d
	jr	nz,2f		; negative increment: shrink, no guard
	jr	c,4f		; wrapped past 64K: fail
	ex	de,hl		; de = new break
	ld	hl,0
	add	hl,sp
	ld	bc,-576
	add	hl,bc		; hl = sp - GUARD
	or	a
	sbc	hl,de		; carry if new break above guard line
	ex	de,hl		; hl = new break
	jr	c,4f		; would grow into the stack: fail
2:
	ld	(_memtop),hl	; commit the new break
	pop	hl		; return old break
	jr	3f
4:
	pop	hl		; unwind saved old break
	ld	hl,-1		; failed
3:
	pop	bc
	ret

	.data
_memtop:
	.dw	0

; vim: tabstop=4 shiftwidth=4 noexpandtab:
