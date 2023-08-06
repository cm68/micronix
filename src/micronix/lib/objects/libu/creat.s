;
;	disas version 3
;	creat.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(24) data: 24(6) bss: 0
;
;	text	0000	0024
;	data	0024	0006
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_creat: LD	HL,H0002	; 0000 !..	  21 02 00 
	ADD	HL,SP		; 0003 9	  39 
H0004: 	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	LD	(H0026),HL	; 0008 "&.	  22 26 00 
	LD	HL,H0004	; 000b !..	  21 04 00 
	ADD	HL,SP		; 000e 9	  39 
	LD	A,(HL)		; 000f ~	  7e 
	INC	HL		; 0010 #	  23 
	LD	H,(HL)		; 0011 f	  66 
	LD	L,A		; 0012 o	  6f 
	LD	(H0028),HL	; 0013 "(.	  22 28 00 
	SYS	indir 24 00 	; 0016 ..$.	  cf 00 24 00 
	LD	C,L		; 001a M	  4d 
	LD	B,H		; 001b D	  44 
	RET	NC		; 001c .	  d0 
	LD	BC,FFFFH	; 001d ...	  01 ff ff 
	LD	(_creat),HL	; 0020 "..	  22 00 00 
	RET			; 0023 .	  c9 


	org	0024H
	DB	CFH,08H
H0026: 	DW	_creat
H0028: 	DW	_creat
