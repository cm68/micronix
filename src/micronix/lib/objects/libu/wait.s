;
;	disas version 3
;	wait.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(1d) data: 1d(0) bss: 0
;
;	text	0000	001d
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_wait: 	PUSH	DE		; 0000 .	  d5 
	SYS	wait 		; 0001 ..	  cf 07 
	JP	C,H0015		; 0003 ...	  da 15 00 
	LD	C,L		; 0006 M	  4d 
	LD	B,H		; 0007 D	  44 
	LD	HL,H0004	; 0008 !..	  21 04 00 
	ADD	HL,SP		; 000b 9	  39 
	LD	A,(HL)		; 000c ~	  7e 
	INC	HL		; 000d #	  23 
	LD	H,(HL)		; 000e f	  66 
	LD	L,A		; 000f o	  6f 
	LD	(HL),E		; 0010 s	  73 
	INC	HL		; 0011 #	  23 
	LD	(HL),D		; 0012 r	  72 
	POP	DE		; 0013 .	  d1 
	RET			; 0014 .	  c9 

H0015: 	POP	DE		; 0015 .	  d1 
	LD	BC,FFFFH	; 0016 ...	  01 ff ff 
	LD	(_wait),HL	; 0019 "..	  22 00 00 
	RET			; 001c .	  c9 

