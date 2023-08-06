;
;	disas version 3
;	exit.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 4 text: 0(12) data: 12(0) bss: 0
;
;	text	0000	0012
;	undef	0100	0003
;
__exit	equ	0100h
c.ret	equ	0101h
c.enô	equ	0102h
;

	org	0000H
_exit: 	CALL	_exit		; 0000 ...	  cd 00 00 
	LD	HL,H0004	; 0003 !..	  21 04 00 
	ADD	HL,DE		; 0006 .	  19 
	LD	C,(HL)		; 0007 N	  4e 
	INC	HL		; 0008 #	  23 
	LD	B,(HL)		; 0009 F	  46 
	PUSH	BC		; 000a .	  c5 
	CALL	_exit		; 000b ...	  cd 00 00 
	POP	AF		; 000e .	  f1 
	JP	_exit		; 000f ...	  c3 00 00 

