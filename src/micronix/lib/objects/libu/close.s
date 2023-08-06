;
;	disas version 3
;	close.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(f) data: f(0) bss: 0
;
;	text	0000	000f
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_close: POP	BC		; 0000 .	  c1 
	POP	HL		; 0001 .	  e1 
	PUSH	HL		; 0002 .	  e5 
	PUSH	BC		; 0003 .	  c5 
	SYS	close 		; 0004 ..	  cf 06 
	LD	BC,_close	; 0006 ...	  01 00 00 
	RET	NC		; 0009 .	  d0 
	DEC	BC		; 000a .	  0b 
	LD	(_close),HL	; 000b "..	  22 00 00 
	RET			; 000e .	  c9 

