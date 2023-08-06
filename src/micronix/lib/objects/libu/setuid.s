;
;	disas version 3
;	setuid.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(13) data: 13(0) bss: 0
;
;	text	0000	0013
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_setuid: LD	HL,H0002	; 0000 !..	  21 02 00 
	ADD	HL,SP		; 0003 9	  39 
	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	SYS	setuid 		; 0008 ..	  cf 17 
	LD	BC,_setuid	; 000a ...	  01 00 00 
	RET	NC		; 000d .	  d0 
	DEC	BC		; 000e .	  0b 
	LD	(_setuid),HL	; 000f "..	  22 00 00 
	RET			; 0012 .	  c9 

