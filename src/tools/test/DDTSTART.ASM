;
;	disas version 3
;	DDT.COM
;
boot	equ	0000h
bdos	equ	0005h
dfcb	equ	005bh
data	equ	067dh

	org	0100h

tpa: 	LD	BC,0FB6H	; ...	    01 b6 0f 
	JP	H013d		; .=.	    c3 3d 01 

	DB	"COPYRIGHT (C) 1978, DIGITAL RESEARCH      "
H0130: 	DB	"DDT VERS 2.0$"
H013d: 	LD	SP,0200H	; 1..	    31 00 02 
	PUSH	BC		; .	    c5 
	PUSH	BC		; .	    c5 
	LD	DE,H0130	; .0.	    11 30 01 
	LD	C,09H		; ..	    0e 09 
	CALL	bdos		; ...	    cd 05 00 
	POP	BC		; .	    c1 
	LD	HL,0007H	; !..	    21 07 00 
	LD	A,(HL)		; ~	    7e 
	DEC	A		; =	    3d 
	SUB	B		; .	    90 
	LD	D,A		; W	    57 
	LD	E,00H		; ..	    1e 00 
	PUSH	DE		; .	    d5 
	LD	HL,0200H	; !..	    21 00 02 
H0158: 	LD	A,B		; x	    78 
	OR	C		; .	    b1 
	JP	Z,H0165		; .e.	    ca 65 01 
	DEC	BC		; .	    0b 
	LD	A,(HL)		; ~	    7e 
	LD	(DE),A		; .	    12 
	INC	DE		; .	    13 
	INC	HL		; #	    23 
	JP	H0158		; .X.	    c3 58 01 

H0165: 	POP	DE		; .	    d1 
	POP	BC		; .	    c1 
	PUSH	HL		; .	    e5 
	LD	H,D		; b	    62 
H0169: 	LD	A,B		; x	    78 
	OR	C		; .	    b1 
	JP	Z,H0187		; ...	    ca 87 01 
	DEC	BC		; .	    0b 
	LD	A,E		; {	    7b 
	AND	07H		; ..	    e6 07 
	JP	NZ,H017a	; .z.	    c2 7a 01 
	EX	(SP),HL		; .	    e3 
	LD	A,(HL)		; ~	    7e 
	INC	HL		; #	    23 
	EX	(SP),HL		; .	    e3 
	LD	L,A		; o	    6f 
H017a: 	LD	A,L		; }	    7d 
	RLA			; .	    17 
	LD	L,A		; o	    6f 
	JP	NC,H0183	; ...	    d2 83 01 
	LD	A,(DE)		; .	    1a 
	ADD	A,H		; .	    84 
	LD	(DE),A		; .	    12 
H0183: 	INC	DE		; .	    13 
	JP	H0169		; .i.	    c3 69 01 

H0187: 	POP	DE		; .	    d1 
	LD	L,00H		; ..	    2e 00 
	JP	(HL)		; .	    e9 

