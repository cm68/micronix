;
;	disas version 3
;	sync.o
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
_sync: 	SYS	sync 		; 0000 .$	  cf 24 
	LD	BC,_sync	; 0002 ...	  01 00 00 
	RET			; 0005 .	  c9 

