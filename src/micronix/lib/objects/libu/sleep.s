;
;	disas version 3
;	sleep.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(e) data: e(0) bss: 0
;
;	text	0000	000e
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_sleep: LD	HL,H0002	; 0000 !..	  21 02 00 
	ADD	HL,SP		; 0003 9	  39 
	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	SYS	sleep 		; 0008 .#	  cf 23 
	LD	BC,_sleep	; 000a ...	  01 00 00 
	RET			; 000d .	  c9 

