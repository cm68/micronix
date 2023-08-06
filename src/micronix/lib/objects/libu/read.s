;
;	disas version 3
;	read.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(2c) data: 2c(6) bss: 0
;
;	text	0000	002c
;	data	002c	0006
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_read: 	LD	HL,H0004	; 0000 !..	  21 04 00 
	ADD	HL,SP		; 0003 9	  39 
H0004: 	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
H0006: 	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	LD	(H002e),HL	; 0008 "..	  22 2e 00 
	LD	HL,H0006	; 000b !..	  21 06 00 
	ADD	HL,SP		; 000e 9	  39 
	LD	A,(HL)		; 000f ~	  7e 
	INC	HL		; 0010 #	  23 
	LD	H,(HL)		; 0011 f	  66 
	LD	L,A		; 0012 o	  6f 
	LD	(H0030),HL	; 0013 "0.	  22 30 00 
	LD	HL,H0002	; 0016 !..	  21 02 00 
	ADD	HL,SP		; 0019 9	  39 
	LD	A,(HL)		; 001a ~	  7e 
	INC	HL		; 001b #	  23 
	LD	H,(HL)		; 001c f	  66 
	LD	L,A		; 001d o	  6f 
	SYS	indir 2c 00 	; 001e ..,.	  cf 00 2c 00 
	LD	C,L		; 0022 M	  4d 
	LD	B,H		; 0023 D	  44 
	RET	NC		; 0024 .	  d0 
	LD	BC,FFFFH	; 0025 ...	  01 ff ff 
	LD	(_read),HL	; 0028 "..	  22 00 00 
	RET			; 002b .	  c9 


	org	002cH
	DB	CFH,03H
H002e: 	DW	_read
H0030: 	DW	_read
