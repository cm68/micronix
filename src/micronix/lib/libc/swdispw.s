;	Wide switch dispatch helper for the code generator
;
;	The byte dispatches live in swdisp.s and are what nearly every
;	switch uses.  This one is for a switch with a case value outside
;	0..255 - "case S_IFDIR" and its neighbours, or a "case -1" for
;	none - where the control has to stay sixteen bits and the
;	comparison with it.  It is a separate object so that a program
;	without such a switch does not link it: the byte helpers are
;	worth their space in almost every program, and this one is not.
;
;	Like them, the table sits inline after the call and is read
;	through the return address, and falling off the end jumps to the
;	byte just past it, which is where the compiler puts the no-match
;	label - so "not found" needs no address stored anywhere.
;
;	Value and label are interleaved here rather than kept in two
;	runs.  The byte version separates them so one cpir-shaped scan
;	can walk the values, and then has to find the label from what
;	the scan left behind; at sixteen bits there is no such scan to
;	protect, and a label sitting immediately after the value it
;	belongs to is found by carrying on reading.  4 + 4n either way.

	psect	text
	global	swtabw

; Sparse dispatch on a sixteen bit value.
;
; Entry:  HL = control value, return address -> table
; Table:  .db n / then n of: .db vlo / .db vhi / .dw label
;
; B is the count, which means borrowing it: B, C and BC are where the
; register allocator puts variables, so it is pushed and given back on
; both ways out.  swtab next door refuses to touch BC at all and pays
; for it by counting in D and E - it can, because its value has been
; narrowed into A and D and E are free.  Here the value needs a pair
; of its own and HL is it, so the only registers left are A and DE,
; and DE is the table walk.  Two bytes for the push and pop is the
; whole cost, and the alternative is not having a counter.
swtabw:
	pop	de		; -> count
	push	bc		; a register home: borrowed, given back
	ld	a,(de)
	ld	b,a		; n
	inc	de		; -> first entry

swtwlp:
	ld	a,(de)		; value, low half
	inc	de
	cp	l
	jr	nz,swtwno
	ld	a,(de)		; value, high half
	cp	h
	jr	z,swtwfd

; no match on this one: de is at the high half, so step over it and
; the label to reach the next entry
swtwno:
	inc	de
	inc	de
	inc	de
	djnz	swtwlp

; nothing matched, and de is now the byte after the table
	pop	bc
	ex	de,hl
	jp	(hl)

; matched, with de at the high half of the value: the label follows it
swtwfd:
	inc	de
	pop	bc
	ld	a,(de)		; label, low half
	inc	de
	ld	h,a		; park it - the control value is dead now
	ld	a,(de)		; label, high half
	ld	l,h
	ld	h,a
	jp	(hl)

; vim: tabstop=4 shiftwidth=4 noexpandtab:
