;	Switch dispatch helpers for the code generator
;
;	A chain of "cp v / jp z,L" costs 5 bytes a case, which over the
;	tree's own 85 switches and 835 cases came to 4175 bytes of pure
;	dispatch.  Both helpers here put the table inline after the call
;	and read it through the return address, so a switch costs the
;	call and the data and nothing else.  Falling off the end of
;	either table jumps to the byte just past it, which is where the
;	compiler puts the no-match label - so "not found" needs no
;	address stored anywhere.
;
;	Case values are bytes, so a value is one byte and a label two.
;	The compiler picks between these and the chain by counting:
;	the chain is 5n, swtab is 4+3n, swidx is 5+2*span.

	psect	text
	global	swtab, swidx

; Sparse dispatch: scan a table of values for A, jump to its label.
;
; Entry:  A = control value, return address -> table
; Table:  .db n / .db v0..v(n-1) / .dw L(n-1)..L0
;
; The scan counts down in E and keeps n in D, so the only registers
; touched are A, D, E and HL.  cpir is the obvious instruction here and
; is a third faster, but its counter is BC - and B, C and BC are where
; the allocator puts register variables.  The chain this replaced
; clobbered nothing but the flags, so a helper that took BC would not
; be the same code: it cost a live variable, which is a wrong answer
; rather than a slow one.  Saving BC around it works and is three
; bytes, but not using it at all is the same size as cpir was and
; leaves no contract to remember.  DE and HL are never register homes,
; and the control value in A is dead once we have jumped.
;
; A match leaves HL just past the byte it matched and E holding n-i.
; The labels start at the end of the values, and stored in reverse the
; one wanted sits at HL+3*(E-1) - which is the whole reason they are
; backwards: forwards, the index would have to be rebuilt from n.
swtab:
	pop	hl		; -> count
	ld	e,(hl)
	ld	d,e		; keep n for the no-match path
	inc	hl		; -> values

swtlp:
	cp	(hl)
	inc	hl
	jr	z,swtfd
	dec	e
	jr	nz,swtlp

; nothing matched: hl is the end of the values, so hl+2n is the end of
; the table and the byte after it is the no-match label
	ld	e,d
	ld	d,0
	add	hl,de
	add	hl,de
	jp	(hl)

swtfd:
	dec	e		; slot = hl + 3*(e-1)
	ld	d,0
	add	hl,de
	add	hl,de
	add	hl,de
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	jp	(hl)

; Dense dispatch: bias A and index straight into a table of labels.
;
; Entry:  A = control value, return address -> table
; Table:  .db lo / .db span / .dw L[0]..L[span-1]
;
; Gaps inside the span hold the no-match label, which is what makes
; this worth choosing only when the values are dense: every hole costs
; the same two bytes as a case.
;
; A value below lo wraps on the subtract into something large, and
; large is out of range, so one unsigned compare covers both ends.
swidx:
	pop	hl		; -> lo
	sub	(hl)
	inc	hl		; -> span
	ld	e,(hl)
	inc	hl		; -> table
	cp	e
	jr	c,swiin
	ld	d,0		; out of range: past the table
	add	hl,de
	add	hl,de
	jp	(hl)

swiin:
	ld	e,a		; 16-bit index: span can exceed 127,
	ld	d,0		; so this cannot be add a,a
	add	hl,de
	add	hl,de
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	jp	(hl)

; vim: tabstop=4 shiftwidth=4 noexpandtab:
