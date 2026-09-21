	psect	data

;============================================================================
; unbcd.s -- packed-BCD calendar time -> Unix seconds.
;
;   Entry:  HL = pointer to 6-byte packed-BCD buffer:
;                [0] year (2-digit: 0x00..0x38 => 2000..2038,
;                                  0x70..0x99 => 1970..1999)
;                [1] month (0x01..0x12)
;                [2] day   (0x01..0x31)
;                [3] hour  (0x00..0x23)
;                [4] minute(0x00..0x59)
;                [5] second(0x00..0x59)
;   Exit:   HL' = high word, HL = low word of Unix seconds since 1970.
;
;   Valid 1970-01-01 .. 2038-01-19 (leap = n&3 == 2).  Clobbers
;   AF, BC, DE, HL, DE', HL'.
;
;   seconds = days*86400 + (hour*60 + minute)*60 + second, where
;       days = 365*n + (n+1)>>2 + day-of-year,  n = years since 1970.
;   86400 = 0x15180 = 0x10000 + 0x5180, so days*86400 = (days<<16)
;   + days*20864 (the latter via a 16x16 multiply).
;============================================================================

	global	bcd2unix

bcd2unix:
	ld	a,(hl)			; year
	inc	hl
	call	bcd2bin
	cp	70
	jr	nc,b2u_yhi
	add	a,30			; 0x00..0x38 => 2000..2038
	jr	b2u_ysv
b2u_yhi:
	sub	70			; 0x70..0x99 => 1970..1999
b2u_ysv:
	ld	(nyr),a

	ld	a,(hl)			; month
	inc	hl
	call	bcd2bin
	ld	(mo),a
	ld	a,(hl)			; day
	inc	hl
	call	bcd2bin
	ld	(da),a
	ld	a,(hl)			; hour
	inc	hl
	call	bcd2bin
	ld	(ho),a
	ld	a,(hl)			; minute
	inc	hl
	call	bcd2bin
	ld	(mi),a
	ld	a,(hl)			; second
	call	bcd2bin
	ld	(se),a

	; ---- time of day -> acc ----
	ld	a,(se)
	ld	(acc+0),a
	xor	a
	ld	(acc+1),a
	ld	(acc+2),a
	ld	(acc+3),a
	ld	a,(mi)
	call	mul60
	ld	de,0
	call	addacc			; acc += minute*60
	ld	a,(ho)
	call	mul60
	call	mul60_16
	call	addacc			; acc += hour*3600

	; ---- days = 365*n + (n+1)>>2 + day-of-year ----
	ld	a,(nyr)
	ld	hl,0
b2u_l365:
	or	a
	jr	z,b2u_d365
	ld	de,365
	add	hl,de
	dec	a
	jr	b2u_l365
b2u_d365:
	ld	a,(nyr)
	inc	a
	srl	a
	srl	a
	ld	e,a
	ld	d,0
	add	hl,de
	ld	(daysl),hl		; 365*n + leaps

	; day-of-year = (day-1) + sum of prior month lengths
	ld	a,(da)
	dec	a
	ld	c,a
	ld	b,0
	ld	a,(mo)
	dec	a
	ld	d,a			; months to add
	ld	e,1			; month number
	ld	hl,mlen
b2u_doy:
	ld	a,d
	or	a
	jr	z,b2u_doydone
	ld	a,e
	cp	2			; February?
	jr	nz,b2u_nofeb
	ld	a,(nyr)
	and	3
	cp	2			; leap?
	jr	nz,b2u_nofeb
	ld	a,(hl)
	inc	a			; 29
	jr	b2u_addl
b2u_nofeb:
	ld	a,(hl)
b2u_addl:
	add	a,c
	ld	c,a
	jr	nc,b2u_noc
	inc	b
b2u_noc:
	dec	d
	inc	hl
	inc	e
	jr	b2u_doy
b2u_doydone:
	ld	hl,(daysl)
	add	hl,bc
	ld	(daysl),hl		; days

	; ---- acc += days*20864 + (days<<16) ----
	ld	bc,20864
	ld	hl,(daysl)
	call	mul16			; de (low) : de' (high) = days*20864
	ld	a,(acc+0)		; add low word
	add	a,e
	ld	(acc+0),a
	ld	a,(acc+1)
	adc	a,d
	ld	(acc+1),a
	exx				; add high word + carry
	ld	a,(acc+2)
	adc	a,e
	ld	(acc+2),a
	ld	a,(acc+3)
	adc	a,d
	ld	(acc+3),a
	exx
	ld	hl,(daysl)		; acc += days<<16
	ld	a,(acc+2)
	add	a,l
	ld	(acc+2),a
	ld	a,(acc+3)
	adc	a,h
	ld	(acc+3),a

	ld	hl,(acc+0)		; result: hl = low, hl' = high
	exx
	ld	hl,(acc+2)
	exx
	ret

;--------------------------------------------------------------------------
; bcd2bin -- A = packed BCD -> A = binary
;--------------------------------------------------------------------------
bcd2bin:
	ld	b,a
	and	0x0f
	ld	c,a
	ld	a,b
	rrca
	rrca
	rrca
	rrca
	and	0x0f
	ld	b,a
	add	a,a
	add	a,a
	add	a,b
	add	a,a
	add	a,c
	ret

;--------------------------------------------------------------------------
; mul60 -- A (0..99) -> HL = A*60
;--------------------------------------------------------------------------
mul60:
	ld	l,a
	ld	h,0
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	ld	e,a
	ld	d,0
	or	a
	sbc	hl,de
	add	hl,hl
	add	hl,hl
	ret

;--------------------------------------------------------------------------
; mul60_16 -- HL (<=1439) -> DE:HL = HL*60 (17 bits)
;--------------------------------------------------------------------------
mul60_16:
	ld	b,h
	ld	c,l
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	or	a
	sbc	hl,bc
	ld	de,0
	add	hl,hl
	rl	e
	add	hl,hl
	rl	e
	ret

;--------------------------------------------------------------------------
; addacc -- acc[4] (little-endian) += DE:HL
;--------------------------------------------------------------------------
addacc:
	ld	a,(acc+0)
	add	a,l
	ld	(acc+0),a
	ld	a,(acc+1)
	adc	a,h
	ld	(acc+1),a
	ld	a,(acc+2)
	adc	a,e
	ld	(acc+2),a
	ld	a,(acc+3)
	adc	a,d
	ld	(acc+3),a
	ret

;--------------------------------------------------------------------------
; mul16 -- BC * HL -> DE (low) : DE' (high).  Uses cnt, preserves BC.
;--------------------------------------------------------------------------
mul16:
	ld	de,0
	exx
	ld	de,0
	exx
	ld	a,16
	ld	(cnt),a
m16_loop:
	sla	e
	rl	d
	exx
	rl	e
	rl	d
	exx
	add	hl,hl
	jr	nc,m16_next
	ld	a,e
	add	a,c
	ld	e,a
	ld	a,d
	adc	a,b
	ld	d,a
	jr	nc,m16_next
	exx
	inc	de
	exx
m16_next:
	ld	a,(cnt)
	dec	a
	ld	(cnt),a
	jr	nz,m16_loop
	ret

;============================================================================
; data
;============================================================================
mlen:	.db	31,28,31,30,31,30,31,31,30,31,30,31

; workspace
se:	.ds	1
mi:	.ds	1
ho:	.ds	1
da:	.ds	1
mo:	.ds	1
nyr:	.ds	1
daysl:	.ds	1
daysh:	.ds	1
cnt:	.ds	1
acc:	.ds	4
