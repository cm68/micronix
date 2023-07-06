;
; cp/m crt0.s
;
; /src/lib/chdr.s
;
; changed: <2023-07-04 11:50:56 curt>
;

bdos	=	0005h

start: 	ld	hl,(0006h)	; get bdos address
	ld	sp,hl		; set stack to it
	call	__main		; call our c main
	push	bc		; get return code
	call	_exit		; call exit

_cpm: 	ld	hl,0002h	; point past return addr
	add	hl,sp		; 
	ld	c,(hl)		; bc = arg0
	inc	hl		;
	ld	b,(hl)		; 
	push	de		; save de
	inc	hl		; 
	ld	e,(hl)		; de = arg1
	inc	hl		; 
	ld	d,(hl)		; 
	inc	hl		; 
	ld	a,(hl)		; hl = arg2
	inc	hl		; 
	ld	h,(hl)		; 
	ld	l,a		; 
	call	bdos		; call into cp/m
	ld	(__hl),hl	; save return values
	ld	l,e		; 
	ld	h,d		; 
	ld	(__de),hl	; 
	ld	l,c		; 
	ld	h,b		; 
	ld	(__bc),hl	; 
	pop	de		; our saved de
	ld	c,a		; 
	add	a,a		; 
	sbc	a,a		; 
	ld	b,a		; bc = sign extended a
	ret			;


	org	0032h
	db	"v2.1: copyright (c) 1979 by whitesmiths, ltd"
	db	"."
__bc: 	dw	start
__de: 	dw	start
__hl: 	dw	start
