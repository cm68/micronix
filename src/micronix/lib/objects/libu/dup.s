;
;	disas version 3
;	dup.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(14) data: 14(0) bss: 0
;
;	text	0000	0014
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_dup: 	LD	HL,H0002	; 0000 !..	  21 02 00 
	ADD	HL,SP		; 0003 9	  39 
	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	SYS	dup 		; 0008 .)	  cf 29 
	LD	C,L		; 000a M	  4d 
	LD	B,H		; 000b D	  44 
	RET	NC		; 000c .	  d0 
	LD	BC,FFFFH	; 000d ...	  01 ff ff 
	LD	(_dup),HL	; 0010 "..	  22 00 00 
	RET			; 0013 .	  c9 

