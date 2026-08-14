; this fragment is called to make a stack frame
; generally, this is the function prolog that happens
; right after a C function is called with stacked args

; here's what the stack looks like when we enter

;        arg1
;        arg0
;        caller's caller return address
; sp->   return to function head

;
; and when we exit, via the jp (hl), the stack and registers looks like this
;
;	  arg1
;         arg0
;         return
;         iy
; ix,sp-> ix
;
; so, arg0 is (ix+6) and (ix+7)
;
	global	csv,cret,indir, ncsv
	psect	text
csv:	pop	hl		;return address
	push	iy
	push	ix
	ld	ix,0
	add	ix,sp		;new frame pointer
	jp	(hl)

cret:	ld	sp,ix
	pop	ix
	pop	iy
	ret

indir:	jp	(hl)

;	ccc's own pair.  The frame above is HiTech's: ix is the frame
;	pointer and both index registers are saved.  ccc points iy at
;	the frame and leaves ix free for the code generator to use as a
;	pointer, so it cannot share csv/cret - the offsets would all be
;	wrong.  What these do is exactly what the compiler was writing
;	out at the top and bottom of every function:
;
;		push iy		2
;		ld iy,0		4
;		add iy,sp	2
;	...
;		ld sp,iy	2
;		pop iy		2
;		ret		1
;
;	thirteen bytes in each of them, against three for a call and
;	three for a jump.  The peephole substitutes these; the frame
;	that results is laid out identically, which is what lets it be
;	a substitution and not a change of calling convention - every
;	(iy+d) the compiler already emitted still means what it did.
;
;	fenter clobbers hl, which holds nothing on entry to a function
;	that takes no arguments - and a function that takes arguments
;	is entered through the w/q variants below, which spill HL before
;	falling in here.  fexit touches neither hl nor the flags, so it
;	can be jumped to with a return value or a condition already set
;	up.
;
;	The frame allocation stays inline: it is a constant the caller
;	knows and one add.  The register saves do not - see below.
;
;	The first argument to a function arrives in HL - in HL':HL when
;	it is a long - and never on the stack.  See HLARG.md.  The w
;	and q entry variants put it back in the slot the caller used to
;	push it into, above the return address, so the frame is laid
;	out exactly as the stack convention's was: arg 1 at (iy+4),
;	arg 2 right above it.  The shuffle lifts both return addresses
;	off the stack - the caller's rides in AF, and nothing between
;	the pop and the push reads a flag - plants the argument, and
;	restores them; at no instant is anything live below SP, so an
;	interrupt cannot eat any of it.  A long goes low word first,
;	the push order pusharg used, which lands the high word at the
;	lower address the way qld expects it.
;
;	The matching w/q exit variants discard the slot: it is above
;	the return address, in stack the caller does not know about -
;	the caller cannot drop what it never pushed - so the exit
;	carries the return address over it through DE, which is dead at
;	every return.

	global	fenter, fexit
	global	fenterw, fenterq

;	The w entries guarantee more than the spill: **hl reaches the
;	function body intact and equal to (iy+4)**.  That is what lets
;	the peephole delete a reload of the first parameter - the rule
;	in peep/rules.c is told this property by name and needs no
;	analysis.  The price is that these cannot share the plain
;	bodies, whose jump target travels in hl; here it ferries
;	through de', which is library scratch, and hl is read back
;	from the slot it just filled.  The q entries make no such
;	promise: a long's high word is in hl' and the reload is not
;	worth the juggling for as rare as long first parameters are.

fenterw:
	pop	de		;the function body
	pop	af		;the caller's return address
	push	hl		;arg 1, into its old slot
	push	af
	push	iy
	ld	iy,0
	add	iy,sp		;new frame pointer
	push	de
	ret			;into the body, hl untouched

fenterq:
	pop	de
	pop	af
	push	hl		;low word, at the higher address
	exx
	push	hl		;high word, at the lower
	exx
	push	af
	push	de
	jr	fenter

fenter:
	pop	hl		;return address
	push	iy
	ld	iy,0
	add	iy,sp		;new frame pointer
	jp	(hl)

fexit:
	ld	sp,iy
	pop	iy
	ret

	global	fexitw, fexitq

fexitw:
	ld	sp,iy
	pop	iy
	pop	de		;the return address
	inc	sp
	inc	sp		;over the spilled first argument
	push	de
	ret

fexitq:
	ld	sp,iy
	pop	iy
	pop	de
	inc	sp
	inc	sp
	inc	sp
	inc	sp		;a long is two slots
	push	de
	ret

;	Callee-save exits.  A function that keeps a variable in BC or in
;	IX has to hand the caller's back, and that was written out at
;	the end of every one of them: twelve bytes for IX, because it
;	has to come back through A to leave HL and the flags alone, and
;	six for BC.  Two thousand bytes of it in c1 alone.
;
;	Here instead, once.  The saved pair sits just under the scalar
;	area, so where it is depends on the frame - the caller says so
;	with a word after the call, the offset of the LOWER save from
;	the frame pointer, and the helper reads them from there.  Five
;	bytes at the call site against twenty-one.
;
;	These used to do their arithmetic in the shadow set, on the
;	grounds that every ordinary register was either the return value
;	or one of the pair being restored.  Two of the three premises
;	were wrong.
;
;	The first: A and the flags are NOT live here.  Every return goes
;	through an assignment to HL - pass2's RETURN case widens even a
;	byte and hands it back in HL - so there is nothing in A to
;	protect and no condition to preserve, and the ex af,af' that
;	bracketed all three was guarding a value that does not exist.
;
;	The second: the saves do not have to be popped.  Pointing SP at
;	them is what forced a spare pair to hold the address; reading
;	them through (hl) does not, because ex (sp),hl parks the return
;	value on the stack and hands over the pointer in the same byte.
;	BC is then free - in two of the three it is about to be
;	restored anyway, and fexx borrows the caller's and gives it
;	back.
;
;	So nothing here touches BC', DE' or HL' any more, which is what
;	lets a long live in HL':HL.  DE is untouched too, so this is
;	correct under either convention.  Seven bytes more across the
;	three of them, once.  _signal still saves the shadow set around
;	a handler; it no longer has to on account of this file.

	global	fexbx, fexx, fexb

fexbx:				;restore IX and BC
	ex	(sp),hl		;hl -> the word after the call,
				; and the return value is on the stack
	ld	c,(hl)
	inc	hl
	ld	b,(hl)		;bc = offset of the lower save from iy
	push	iy
	pop	hl
	add	hl,bc		;-> the saves
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	bc
	pop	ix		;pushed last, so it is underneath
	inc	hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	pop	hl		;the return value back
	jr	fexit

fexx:				;restore IX alone
	ex	(sp),hl
	push	bc		;the caller's, borrowed and given back
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	iy
	pop	hl
	add	hl,bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	bc
	pop	ix
	pop	bc
	pop	hl
	jr	fexit

fexb:				;restore BC alone
	ex	(sp),hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	iy
	pop	hl
	add	hl,bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	pop	hl
	jr	fexit

;	The same three, for a function whose first argument came in HL
;	(w) or HL':HL (q): the entry spilled it above the return
;	address, and these discard it on the way out.  The bodies are
;	the ones above verbatim - only the unwind they leave through
;	differs - and jp rather than jr because fexitw sits out of
;	range from the far end of this run.

	global	fexbxw, fexxw, fexbw
	global	fexbxq, fexxq, fexbq

fexbxw:				;restore IX and BC
	ex	(sp),hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	iy
	pop	hl
	add	hl,bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	bc
	pop	ix
	inc	hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	pop	hl
	jp	fexitw

fexbxq:
	ex	(sp),hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	iy
	pop	hl
	add	hl,bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	bc
	pop	ix
	inc	hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	pop	hl
	jp	fexitq

fexxw:				;restore IX alone
	ex	(sp),hl
	push	bc		;the caller's, borrowed and given back
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	iy
	pop	hl
	add	hl,bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	bc
	pop	ix
	pop	bc
	pop	hl
	jp	fexitw

fexxq:
	ex	(sp),hl
	push	bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	iy
	pop	hl
	add	hl,bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	bc
	pop	ix
	pop	bc
	pop	hl
	jp	fexitq

fexbw:				;restore BC alone
	ex	(sp),hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	iy
	pop	hl
	add	hl,bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	pop	hl
	jp	fexitw

fexbq:
	ex	(sp),hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	push	iy
	pop	hl
	add	hl,bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	pop	hl
	jp	fexitq

;	And the entries that match them.  A prologue was the frame
;	pointer (three, through fenter), the scalar area (five, and it
;	is a constant), and the pushes (three) - eleven bytes, of which
;	only the constant is particular to the function.  These take it
;	as the word after the call, so the whole prologue is five.
;
;	The order matters and is the old one exactly: frame pointer,
;	then the scalar area, then the saves just under it, so that the
;	saves stay inside the 7-bit (iy+d) window and everything the
;	compiler has already emitted still means what it did.  Arrays
;	are allocated by the caller afterwards; they live below the
;	saves and are reached by arithmetic, not by (iy+d).

	global	fentbx, fentx, fentb, fentn
	global	fentbxw, fentxw, fentbw, fentnw
	global	fentbxq, fentxq, fentbq, fentnq

;	The w and q entries: the first argument is in HL (HL':HL for a
;	long), and goes back into its old slot above the caller's
;	return address before the plain helper builds the frame it has
;	always built.  Both return addresses come off the stack first -
;	the helper's own, which is the pointer to the word, rides in
;	DE; the caller's rides in AF, and nothing here reads a flag
;	before the push puts it back.

;	The shared shape of the four worded w entries.  The scalar-area
;	word is read as its low byte and sign-extended: assignFrmOff
;	caps the scalar area at 120, so the high byte says nothing.
;	The body address ferries through de' - bc' would do as well,
;	but de':de is the second accumulator and certainly dead - and
;	hl comes back from the slot, so the promise above holds however
;	big the frame is.

fentbxw:			;frame, scalar area, save IX and BC
	pop	de		;-> the word after the call
	pop	af		;the caller's return address
	push	hl		;arg 1, into its old slot
	push	af
	push	iy
	ld	iy,0
	add	iy,sp		;new frame pointer
	ld	a,(de)		;-savebase, the low byte says it all
	inc	de
	inc	de		;de = the body
	push	de
	exx
	pop	de		;the body address, parked in de'
	exx
	ld	l,a
	add	a,a
	sbc	a,a
	ld	h,a		;hl = -savebase, sign extended
	add	hl,sp
	ld	sp,hl		;scalar area allocated
	push	bc
	push	ix
	ld	l,(iy+4)
	ld	h,(iy+5)	;arg 1 back: hl and its slot agree
	exx
	push	de		;the body address, off the ferry
	exx
	ret			;and into it

fentxw:				;the same, saving IX alone
	pop	de
	pop	af
	push	hl
	push	af
	push	iy
	ld	iy,0
	add	iy,sp
	ld	a,(de)
	inc	de
	inc	de
	push	de
	exx
	pop	de
	exx
	ld	l,a
	add	a,a
	sbc	a,a
	ld	h,a
	add	hl,sp
	ld	sp,hl
	push	ix
	ld	l,(iy+4)
	ld	h,(iy+5)
	exx
	push	de
	exx
	ret

fentbw:				;saving BC alone
	pop	de
	pop	af
	push	hl
	push	af
	push	iy
	ld	iy,0
	add	iy,sp
	ld	a,(de)
	inc	de
	inc	de
	push	de
	exx
	pop	de
	exx
	ld	l,a
	add	a,a
	sbc	a,a
	ld	h,a
	add	hl,sp
	ld	sp,hl
	push	bc
	ld	l,(iy+4)
	ld	h,(iy+5)
	exx
	push	de
	exx
	ret

fentnw:				;no saves, but a scalar area to allocate
	pop	de
	pop	af
	push	hl
	push	af
	push	iy
	ld	iy,0
	add	iy,sp
	ld	a,(de)
	inc	de
	inc	de
	push	de
	exx
	pop	de
	exx
	ld	l,a
	add	a,a
	sbc	a,a
	ld	h,a
	add	hl,sp
	ld	sp,hl
	ld	l,(iy+4)
	ld	h,(iy+5)
	exx
	push	de
	exx
	ret

;	The q entries spill both halves - low word first, so the high
;	word lands at the lower address the way qld expects - and fall
;	into the plain bodies: no hl promise, see above.

fentbxq:
	pop	de
	pop	af
	push	hl		;low word, at the higher address
	exx
	push	hl		;high word, at the lower
	exx
	push	af
	push	de
	jr	fentbx

fentxq:
	pop	de
	pop	af
	push	hl
	exx
	push	hl
	exx
	push	af
	push	de
	jr	fentx

fentbq:
	pop	de
	pop	af
	push	hl
	exx
	push	hl
	exx
	push	af
	push	de
	jr	fentb

fentnq:
	pop	de
	pop	af
	push	hl
	exx
	push	hl
	exx
	push	af
	push	de
	jr	fentn

fentbx:				;frame, scalar area, save IX and BC
	pop	hl		;-> the word after the call
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl		;hl = return address, de = -scalars
	push	iy
	ld	iy,0
	add	iy,sp		;new frame pointer
	ex	de,hl
	add	hl,sp
	ld	sp,hl		;scalar area allocated
	ex	de,hl		;hl = return address
	push	bc
	push	ix
	jp	(hl)

fentx:				;the same, saving IX alone
	pop	hl
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	push	iy
	ld	iy,0
	add	iy,sp
	ex	de,hl
	add	hl,sp
	ld	sp,hl
	ex	de,hl
	push	ix
	jp	(hl)

fentb:				;saving BC alone
	pop	hl
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	push	iy
	ld	iy,0
	add	iy,sp
	ex	de,hl
	add	hl,sp
	ld	sp,hl
	ex	de,hl
	push	bc
	jp	(hl)

fentn:				;no saves, but a scalar area to allocate
	pop	hl
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	push	iy
	ld	iy,0
	add	iy,sp
	ex	de,hl
	add	hl,sp
	ld	sp,hl
	ex	de,hl
	jp	(hl)

;	There is no helper for saving BC in a hand-written routine,
;	and there deliberately is not.  One was tried: it parked the
;	saved BC (and, once that bug was found, the caller's return
;	address) in a static side stack, because the bodies here read
;	their arguments by popping and a save pushed at entry would
;	come back as an argument.  But a static side stack is state
;	the real stack does not know about: a longjmp past a frame
;	that is midway through such a routine leaves the side stack
;	pointing at dead slots, and the next restore hands back
;	garbage.  A signal handler that touches stdio has the same
;	problem.
;
;	The rule instead: a hand-written routine that needs BC saves
;	it on the real stack and reads its arguments where they sit,
;	indexed past the saves and the return address.  strcmp,
;	strcpy, fgetc and fputc are the pattern.  Everything lives on
;	the one stack, so setjmp owes nothing to anybody.

;	New csv: allocates space for stack based on word following
;	call ncsv

ncsv:
	pop	hl
	push	iy
	push	ix
	ld	ix,0
	add	ix,sp
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	ex	de,hl
	add	hl,sp
	ld	sp,hl
	ex	de,hl
	jp	(hl)

; vim: tabstop=4 shiftwidth=4 noexpandtab:

