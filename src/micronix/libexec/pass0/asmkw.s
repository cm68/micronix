;
; kwlook - look up a keyword in a skip-optimized trie
;
; The hand-tuned replacement for the portable C kwlook in kw.c.  It is
; the same algorithm translated one-for-one, in one routine because the
; inline-asm form had to be split across several asm{} blocks and the
; register state did not survive the split.
;
; Calling convention (ccc): the string arrives in HL, the table is read
; at [sp+2], and the token is returned in HL.  BC and IX are the
; caller's register variables, so BC is saved.
;
; Registers through the body:
;	hl - the table pointer, walking
;	bc - the original string, for the resets
;	de - the string pointer, walking
;	a  - scratch
;
	psect	text
	global	_kwlook

_kwlook:
	ex	de,hl			; de = string (first arg)
	ld	hl,2
	add	hl,sp			; past the return address
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a			; hl = table (second arg)
	push	bc			; the caller's register variable
	ld	b,d
	ld	c,e			; bc = string, for the resets

;
; Classify the table byte: 0 ends the table, 0xff/0xfe terminate a
; word, bit 7 marks a skip node, anything else is a plain character.
;
loop:
	ld	a,(hl)
	or	a
	jr	z,notfound		; table exhausted
	cp	0xff
	jr	z,termff
	cp	0xfe
	jr	z,termfe
	bit	7,a
	jr	z,literal

;
; A skip node: match the low seven bits and step over the skip byte,
; or jump the whole sibling on a mismatch.
;
	and	0x7f
	ex	de,hl
	cp	(hl)			; the character vs the string
	ex	de,hl
	jr	nz,skipnode
	inc	hl
	inc	hl			; table += 2 (char|0x80 and its skip)
	inc	de			; string += 1
	jr	loop
skipnode:
	inc	hl			; hl -> the skip distance
	ld	a,(hl)
	inc	a			; +1 because hl already stepped one
	add	a,l
	ld	l,a
	ld	a,0
	adc	a,h
	ld	h,a			; table += skip + 1
	jr	loop

;
; 0xff ends a complete word: a spent string returns the token, an
; unspent one resets and tries the next entry.  0xfe is a prefix
; terminator: keep the string for the longer pattern.
;
termff:
	ld	a,(de)
	or	a
	jr	nz,ffreset
	inc	hl
	ld	a,(hl)			; the token
	jr	done
ffreset:
	inc	hl
	inc	hl			; past 0xff and its token
	ld	d,b
	ld	e,c			; string = the original again
	jr	loop
termfe:
	ld	a,(de)
	or	a
	jr	nz,fecont
	inc	hl
	ld	a,(hl)			; the token
	jr	done
fecont:
	inc	hl
	inc	hl			; past 0xfe and its token, string kept
	jr	loop

;
; A plain character must match or the entry is a miss.
;
literal:
	ex	de,hl
	cp	(hl)
	ex	de,hl
	jr	nz,scan
	inc	hl
	inc	de
	jr	loop

;
; Recover from a literal miss: step to the entry's terminator, reset
; the string, and try the next entry - or report not-found.
;
scan:
	ld	a,(hl)
	cp	0xff
	jr	z,scandone
	cp	0xfe
	jr	z,scandone
	or	a
	jr	z,notfound
	bit	7,a
	jr	z,scan1
	inc	hl
	inc	hl			; a skip node: char|0x80 and its skip
	jr	scan
scan1:
	inc	hl			; a plain character
	jr	scan
scandone:
	inc	hl
	inc	hl			; past the terminator and its token
	ld	d,b
	ld	e,c			; string = the original again
	jr	loop

notfound:
	ld	a,0xff
done:
	ld	l,a
	ld	h,0
	pop	bc
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
