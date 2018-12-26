;	16 bit integer multiply

;	on entry, left operand is in hl, right operand in de
;	result in hl.

	psect	text
	global	amul,lmul
amul:
lmul:
	ld	a,e		; save d,e in c,a
	ld	c,d
	ex	de,hl		; save hl in de
	ld	hl,0		; 16 bit accumulator
	ld	b,8		; byte count
	call	mult8b		; hl = left * low(right)
	ex	de,hl
	jr	3f
2:	add	hl,hl
3:
	djnz	2b		; shift de for remaining bits
	ex	de,hl
1:
	ld	a,c		; now low
mult8b:		
	srl	a		; low bit one?
	jp	nc,1f
	add	hl,de		; add de to accumulator
1:	ex	de,hl
	add	hl,hl		; shift de left
	ex	de,hl
	ret	z		; if no more set bits
	djnz	mult8b		; do 8
	ret
