;	16 bit integer multiply

;	on entry, left operand is in hl, right operand in de
;	result in hl.

	psect	text
	global	amul,lmul
;
;	BC is callee-save.  This routine keeps its byte counter in B and
;	the right operand's high half in C, and it cannot hand the
;	caller's back from inside: mult8b is CALLED for the first eight
;	bits and FALLEN INTO for the second, so the ret that ends the
;	routine is the same ret that returns from that call.  There is no
;	single exit to restore at, so the save goes outside and the body
;	is left exactly as it was.
;
;	Until this, "x * y" destroyed whatever the caller was keeping in
;	BC.  Nothing noticed because every compiled function saves BC in
;	its own prologue whether it needs to or not - which is the cost
;	this pays for.
;
amul:
lmul:
	push	bc
	call	imulw
	pop	bc
	ret
imulw:
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

; vim: tabstop=4 shiftwidth=4 noexpandtab:
