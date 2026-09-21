	psect	data

;============================================================================
; rtcglue.s -- C-callable boundary for the RTC date-math.
;
; Both leaf functions speak ccc's long ABI directly: a long lives in
; HL':HL (HL = low word, HL' = high word).  The frame helpers
; fenter/fexit live in csv.s (the u page).
;
; Each wraps a single leaf function, so the year-scan and the BCD field
; reordering stay in rtc.c where they are easy to read; these are just
; the frame <-> register boundary.
;============================================================================

	global	_bcd2unix, _unixtobcd

; ---- long bcd2unix(UINT8 *b) -> unix seconds ----
_bcd2unix:
	call	fenterw		; hl = b (arg 1), frame set up
	call	bcd2unix	; HL = low, HL' = high (ccc long ABI)
	call	fexitw		; drop the spilled arg, return

; ---- void unixtobcd(long t, UINT8 *b) ----
_unixtobcd:
	call	fenterq		; t spilled: (iy+4)=hi,(iy+6)=lo,(iy+8)=b
	ld	l,(iy+6)
	ld	h,(iy+7)	; HL = low
	exx
	ld	l,(iy+4)
	ld	h,(iy+5)	; HL' = high
	exx
	ld	e,(iy+8)
	ld	d,(iy+9)	; DE = b
	call	unixtobcd	; fills b
	call	fexitq		; drop the long's two slots, return
