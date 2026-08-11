;
; brk/sbrk - user-space break tracking over the _break system call
;
; memtop holds the current break, lazily started at the end of bss
; (__Hbss).  sbrk(n) grows the break by n and returns the OLD break
; (base of the granted region); sbrk(0) returns the current break.
; malloc depends on both.  brk(addr) sets the break, 0/-1 result.
;
; BC is preserved: hitech and ccc keep register variables there.
;
	.extern __break, __Hbss
	.global _brk, _sbrk, _memtop

; refuse to grow the break to within GUARD bytes of the current
; stack pointer.  malloc sees -1 and returns NULL instead of the
; heap silently growing up through the stack.
;
; 576: the deepest stack the self-build has been seen to use is 546
; bytes (sim -S, c0 over its own sources), and the old 1024 was
; refusing allocations while nineteen hundred bytes still lay
; between the break and the stack's low-water mark.  The compiler
; lives at the edge of this machine; a guard costs heap.

	.text
_brk:
	pop	de		; return address
	pop	hl		; addr
	push	hl
	push	de
	push	hl		; arg for _break
	call	__break
	pop	de		; clean arg; de = addr
	ld	a,h
	or	l
	ret	nz		; failed: hl = -1
	ld	(_memtop),de
	ret			; hl = 0

_sbrk:
	push	bc		; callee-saved (C register variables)
	ld	hl,4		; [bc][ret][increment]
	add	hl,sp
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		; de = increment
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
	push	hl		; arg: new break
	call	__break
	pop	de		; clean arg; de = new break
	ld	a,h
	or	l
	jr	nz,4f
	ld	(_memtop),de
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

; vim: tabstop=8 shiftwidth=8 noexpandtab:
