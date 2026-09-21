	psect	data

;============================================================================
; unixtobcd.s -- Unix seconds -> packed-BCD calendar time.
;
;   Entry:  HL' = high word, HL = low word of Unix seconds since 1970.
;           DE = pointer to 6-byte output buffer.
;   Exit:   buffer: [0] year (2-digit BCD)  [1] month  [2] day
;                   [3] hour  [4] minute  [5] second
;   Valid 1970-01-01 .. 2038-01-19 (leap = year&3 == 0).  Clobbers
;   AF, BC, DE, HL, IX, HL'.
;
;   Each divisor in C is < 128, so the 8-bit remainder register in
;   div32x8 never overflows.  seconds = days*86400 + sod, and days is
;   found by dividing by 60, 60, 24.
;============================================================================

	global	unixtobcd

unixtobcd:
	push	de
	pop	ix			; ix = output buffer

	ld	c,60			; second = t % 60
	call	div32x8
	call	bcd8
	ld	(ix+5),a
	ld	c,60			; minute = (t/60) % 60
	call	div32x8
	call	bcd8
	ld	(ix+4),a
	ld	c,24			; hour = (t/3600) % 24, days = t/86400
	call	div32x8
	call	bcd8
	ld	(ix+3),a
					; hl = days (hl' is now 0)

	ld	e,0xb2			; 1970, low byte
cbt_yloop:
	ld	a,e
	and	3
	ld	bc,365
	jr	nz,cbt_len
	inc	bc			; 366
cbt_len:
	or	a
	sbc	hl,bc
	jr	c,cbt_yrdone
	inc	e			; year++
	jr	cbt_yloop
cbt_yrdone:
	add	hl,bc			; hl = day-of-year (0-based)
	ld	a,e
	exx
	ld	e,a			; stash year in e' (frees de for the table)
	exx

	ld	de,mlen
	ld	b,0
cbt_mloop:
	ld	a,(de)
	inc	de
	ld	c,a
	inc	b			; month 1..12
	ld	a,b
	cp	2
	jr	nz,cbt_nofeb
	exx
	ld	a,e			; year, from e'
	and	3
	exx
	jr	nz,cbt_nofeb
	inc	c			; leap February = 29
cbt_nofeb:
	ld	a,h
	or	a
	jr	nz,cbt_subm
	ld	a,l
	cp	c
	jr	c,cbt_mdone
cbt_subm:
	ld	a,l
	sub	c
	ld	l,a
	ld	a,h
	sbc	a,0
	ld	h,a
	jr	cbt_mloop
cbt_mdone:
	ld	a,b
	call	bcd8
	ld	(ix+1),a		; month
	ld	a,l
	inc	a
	call	bcd8
	ld	(ix+2),a		; day

	exx
	ld	a,e			; year, from e'
	exx
	call	yr100
	call	bcd8
	ld	(ix+0),a		; year (2-digit)
	ret

;--------------------------------------------------------------------------
; div32x8 -- HL':HL / C -> quotient HL':HL, remainder A.  C must be < 128.
;--------------------------------------------------------------------------
div32x8:
	ld	b,32
	xor	a
d32_loop:
	or	a
	rl	l
	rl	h
	exx
	rl	l
	rl	h
	exx
	rla
	cp	c
	jr	c,d32_next
	sub	c
	set	0,l
d32_next:
	djnz	d32_loop
	ret

;--------------------------------------------------------------------------
; bcd8 -- A (0..99) -> A = packed BCD
;--------------------------------------------------------------------------
bcd8:
	ld	c,0
b8_loop:
	cp	10
	jr	c,b8_done
	sub	10
	inc	c
	jr	b8_loop
b8_done:
	ld	b,a
	ld	a,c
	add	a,a
	add	a,a
	add	a,a
	add	a,a
	or	b
	ret

;--------------------------------------------------------------------------
; yr100 -- year low byte (0xb2..0xf6) -> year % 100
;--------------------------------------------------------------------------
yr100:
	cp	208
	jr	nc,yr_ge
	sub	108			; 1970..1999 -> 70..99
	ret
yr_ge:
	sub	208			; 2000..2038 -> 0..38
	ret

;============================================================================
; data
;============================================================================
mlen:	.db	31,28,31,30,31,30,31,31,30,31,30,31
