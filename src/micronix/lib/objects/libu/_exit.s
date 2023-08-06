;
;	disas version 3
;	_exit.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(6) data: 6(0) bss: 0
;
;	text	0000	0006
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
__exit: POP	BC		; 0000 .	  c1 
	POP	HL		; 0001 .	  e1 
	PUSH	HL		; 0002 .	  e5 
	PUSH	BC		; 0003 .	  c5 
	SYS	exit 		; 0004 ..	  cf 01 
