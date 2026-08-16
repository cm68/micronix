;	Long (32-bit) load and store through a pointer
;
;	See QLONG.md for the convention.  These replace lld/lldde/lstde,
;	which were ccc's own - zc3 loads and stores a long inline and
;	never called them - so unlike the arithmetic there is no Hi-Tech
;	version to keep alongside.

	psect	text
	global	qld, qldde, qst
	global	qldiy, qldix, qstiy, qstix

;	A long in a frame slot or through a struct pointer, which is
;	where nearly all of them are: 166 sites on IY and 31 on IX
;	across cpp, pass1 and pass2.
;
;	Written out, that is four byte-moves and an exx pair - fourteen
;	bytes, because (index+d) reaches one byte at a time and the two
;	halves are in different banks.  Building the address for the
;	plain qld instead costs seven before the call.
;
;	So the displacement rides inline, after the call, the way the
;	frame helpers in csv.s take theirs:
;
;		call	qldiy
;		.db	-4
;
;	Four bytes against fourteen.  The helper pops the return
;	address, reads the byte, sign-extends it, adds the index
;	register and falls into the ordinary qld or qst.
;
;	A signed byte is enough: pass1 caps the scalar area at 120 and
;	arrays live below the callee-save gap, reached by arithmetic
;	rather than as an INDEX, so an index offset is always inside the
;	7-bit window the (index+d) forms need anyway.
;
;	Both index registers, because structs contain longs - the AST
;	node and the symbol table entry both do - and a register-homed
;	pointer walking them is exactly what IX is for.

qldiy:
	push	iy
	jr	1f
qldix:
	push	ix
1:
	pop	hl		;the index register
	pop	de		;-> the displacement byte
	ld	a,(de)
	inc	de
	push	de		;the return address, past it
	ld	e,a
	rla			;bit 7 of the displacement into carry
	sbc	a,a		;00 or ff
	ld	d,a		;de = the displacement, sign extended
	add	hl,de
	jp	qld

;	The store cannot take the address in HL - HL is the low word of
;	the value - so it builds qst's stack frame instead and jumps in.
;	ex (sp),hl does the swapping without a spare pair.
qstiy:
	push	iy
	jr	2f
qstix:
	push	ix
2:
	pop	de		;the index register
	ex	(sp),hl		;hl -> the displacement byte, low word parked
	ld	a,(hl)
	inc	hl		;the real return address
	push	hl
	ld	l,a
	rla
	sbc	a,a
	ld	h,a		;hl = the displacement, sign extended
	add	hl,de		;hl = the address
	pop	de		;the return address back
	ex	(sp),hl		;hl = the low word again, address on the stack
	push	de		;and qst's frame is complete
	jp	qst

; Load 32-bit from (HL) into HL':HL
; Entry: HL = pointer to the long
; Exit:  HL' = high word, HL = low word
; Clobbers: DE, A.  DE':DE must be dead.
;
; The pointer sits in the pair the low word has to end up in, so the
; value is assembled in HL and DE first and the high half handed over
; to HL' through the stack.
qld:
	ld	e,(hl)		;high word, from the lower address
	inc	hl
	ld	d,(hl)
	inc	hl
	ld	a,(hl)		;low word, from the higher one
	inc	hl
	ld	h,(hl)
	ld	l,a		;hl = low word, de = high word
	push	de
	exx
	pop	hl		;hl' = high word
	exx
	ret

; Load 32-bit from (DE) into HL':HL
qldde:
	ex	de,hl
	jp	qld

; Store HL':HL through the pointer on the stack
; Entry: HL':HL = the value; the dest pointer is a pushed argument,
;        so it sits above the return address
; Exit:  stored, pointer consumed, and the value still in HL':HL
; Clobbers: DE'.  DE, A and BC come through untouched.
;
; BC being alive is not incidental.  It is the register-variable home,
; and an early version of the Hi-Tech lstde parked the return address
; there: every long store made by a function keeping a pointer in BC -
; tokcpy, with the destination token - handed that pointer back as a
; text address, and the fields stored after the long went through it
; into whatever it now named.  That was cpp's intern pool, six-byte
; token strides at a time.  The fix after that used a scratch word in
; .data, which has no reentrancy at all: a longjmp past a frame midway
; through, or a signal handler that reached stdio, left it holding a
; dead address.
;
; Neither is needed here.  The whole thing runs in the shadow bank,
; where BC' is free and holds the return address, and the low word -
; which starts in the main bank, the wrong side of the exx - comes
; across on the stack.
;
; The parity is the thing to keep hold of.  Between the exx below and
; the one at the end, the register named hl is the HIGH word and the
; LOW word is sitting untouched in the prime, which is exactly where
; the final exx needs it to be.
qst:
	push	hl		;the low word.  stack: low, ret, ptr
	exx			;hl = high word; the low one is safe in hl'
	pop	de		;de' = low word.    stack: ret, ptr
	pop	bc		;bc' = return addr. stack: ptr
	ex	(sp),hl		;hl = dest pointer; the high word is on the stack
	inc	hl
	inc	hl
	inc	hl		;hl -> the last byte of the low word
	ld	(hl),d
	dec	hl
	ld	(hl),e		;low word up, at the higher address
	dec	hl
	pop	de		;de' = high word.   stack: as the caller left it
	ld	(hl),d
	dec	hl
	ld	(hl),e		;high word down, at the lower address
	ex	de,hl		;hl = high word again
	push	bc		;the return address, for the ret below
	exx			;hl = low word, hl' = high word, bc the caller's
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
