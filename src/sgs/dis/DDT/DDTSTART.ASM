;	disas version 3
boot	equ	0000h
bdos	equ	0005h
dfcb	equ	005bh

	org	0100h

tpa: 	LD	BC,0FB6H	; ...	    01 b6 0f 		; size of ddt
	JP	H013d		; .=.	    c3 3d 01 

	DB	"COPYRIGHT (C) 1978, DIGITAL RESEARCH      "
H0130: 	DB	"DDT VERS 2.0$"
H013d: 	LD	SP,0200H	; 1..	    31 00 02 
	PUSH	BC		; .	    c5 			; save stuff
	PUSH	BC		; .	    c5 
	LD	DE,H0130	; .0.	    11 30 01		; signon 
	LD	C,09H		; ..	    0e 09 
	CALL	bdos		; ...	    cd 05 00 

	POP	BC		; .	    c1 
	LD	HL,0007H	; !..	    21 07 00 		; get bdos page
	LD	A,(HL)		; ~	    7e 
	DEC	A		; =	    3d 			; bdos - 1
	SUB	B		; .	    90 			; subtract 15
	LD	D,A		; W	    57 			; this is where we are copying to
	LD	E,00H		; ..	    1e 00 
	PUSH	DE		; .	    d5 			; save destaddr
	LD	HL,0200H	; !..	    21 00 02 		; this is where we are copying from

copy: 	LD	A,B		; x	    78 			; is BC == 0?
	OR	C		; .	    b1 
	JP	Z,H0165		; .e.	    ca 65 01 

	DEC	BC		; .	    0b 
	LD	A,(HL)		; ~	    7e 			; get a byte
	LD	(DE),A		; .	    12 			; store a byte
	INC	DE		; .	    13 
	INC	HL		; #	    23 
	JP	copy		; .X.	    c3 58 01 		; more

H0165: 	POP	DE		; .	    d1 			; get dest addr back
	POP	BC		; .	    c1 			; get ddt size
	PUSH	HL		; .	    e5 			; save ddt code end

	LD	H,D		; b	    62 

patch: 	LD	A,B		; x	    78 			; is BC == 0?
	OR	C		; .	    b1 
	JP	Z,done		; ...	    ca 87 01 
	DEC	BC		; .	    0b 

	LD	A,E		; {	    7b 			; is 8 byte aligned?
	AND	07H		; ..	    e6 07 
	JP	NZ,H017a	; .z.	    c2 7a 01 		; not aligned

	EX	(SP),HL		; .	    e3 			; inc tos
	LD	A,(HL)		; ~	    7e 			; get (tos)
	INC	HL		; #	    23 
	EX	(SP),HL		; .	    e3 

	LD	L,A		; o	    6f 

H017a: 	LD	A,L		; }	    7d 
	RLA			; .	    17 			; if high bit set, patch
	LD	L,A		; o	    6f 
	JP	NC,nopat	; ...	    d2 83 01 
	LD	A,(DE)		; .	    1a 			; get code
	ADD	A,H		; .	    84 			; add page offset
	LD	(DE),A		; .	    12			; write code 	
nopat: 	INC	DE		; .	    13 			; bump code ptr
	JP	patch		; .i.	    c3 69 01 

done: 	POP	DE		; .	    d1 
	LD	L,00H		; ..	    2e 00 
	JP	(HL)		; .	    e9 

