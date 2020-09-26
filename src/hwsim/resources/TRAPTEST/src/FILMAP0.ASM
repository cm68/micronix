;
;	disas version 3
;	FILMAP0.COM
;
superlo	equ	0001h
superhi	equ	0002h
ctask	equ	0006h
cmask	equ	0007h
pcsavelo	equ	0015h
pcsavehi	equ	0016h
umask	equ	0021h
statusmask	equ	0403h
PUTMAP	equ	0806h
WHAT	equ	0812h
RESTR	equ	0815h

	org	01000h

H1000: 	HALT			; v	    76 
	HALT			; v	    76 
	LD	A,00H		; >.	    3e 00 
H1004: 	LD	B,00H		; ..	    06 00 
	LD	C,03H		; ..	    0e 03 
H1008: 	CALL	PUTMAP		; ...	    cd 06 08 
	INC	A		; <	    3c 
	INC	B		; .	    04 
	BIT	4,B		; .`	    cb 60 
	JP	Z,H1008		; ...	    ca 08 10 
	CP	00H		; ..	    fe 00 
	JP	NZ,H1004	; ...	    c2 04 10 
	LD	DE,0019H	; ...	    11 19 00 
	LD	HL,pcsavelo	; !..	    21 15 00 
	LD	B,00H		; ..	    06 00 
	LD	A,00H		; >.	    3e 00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
H1024: 	LD	C,01H		; ..	    0e 01 
H1026: 	LD	(HL),B		; p	    70 
	INC	HL		; #	    23 
	LD	(HL),A		; w	    77 
	ADD	HL,DE		; .	    19 
	INC	C		; .	    0c 
	BIT	4,C		; .a	    cb 61 
	JP	Z,H1026		; .&.	    ca 26 10 
	LD	A,19H		; >.	    3e 19 
	LD	HL,umask	; !!.	    21 21 00 
	LD	DE,001AH	; ...	    11 1a 00 
	LD	C,FFH		; ..	    0e ff 
H103a: 	LD	(HL),A		; w	    77 
	ADD	HL,DE		; .	    19 
	INC	C		; .	    0c 
	BIT	4,C		; .a	    cb 61 
	JP	Z,H103a		; .:.	    ca 3a 10 
	LD	A,01H		; >.	    3e 01 
	LD	(ctask),A	; 2..	    32 06 00 
	JP	RESTR		; ...	    c3 15 08 

	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H
H1100: 	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
	NOP			; .	    00 
