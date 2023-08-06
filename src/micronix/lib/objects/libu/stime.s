;
;	disas version 3
;	stime.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(1e) data: 1e(0) bss: 0
;
;	text	0000	001e
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_stime: PUSH	DE		; 0000 .	  d5 
	LD	HL,H0004	; 0001 !..	  21 04 00 
H0004: 	ADD	HL,SP		; 0004 9	  39 
	LD	A,(HL)		; 0005 ~	  7e 
	INC	HL		; 0006 #	  23 
	LD	H,(HL)		; 0007 f	  66 
	LD	L,A		; 0008 o	  6f 
	LD	E,(HL)		; 0009 ^	  5e 
	INC	HL		; 000a #	  23 
	LD	D,(HL)		; 000b V	  56 
	INC	HL		; 000c #	  23 
	LD	A,(HL)		; 000d ~	  7e 
	INC	HL		; 000e #	  23 
	LD	H,(HL)		; 000f f	  66 
	LD	L,A		; 0010 o	  6f 
	EX	DE,HL		; 0011 .	  eb 
	SYS	stime 		; 0012 ..	  cf 19 
	POP	DE		; 0014 .	  d1 
	LD	BC,_stime	; 0015 ...	  01 00 00 
	RET	NC		; 0018 .	  d0 
	DEC	BC		; 0019 .	  0b 
	LD	(_stime),HL	; 001a "..	  22 00 00 
	RET			; 001d .	  c9 

