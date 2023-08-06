;
;	disas version 3
;	getpid.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(5) data: 5(0) bss: 0
;
;	text	0000	0005
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_getpid: SYS	getpid 		; 0000 ..	  cf 14 
	LD	C,L		; 0002 M	  4d 
	LD	B,H		; 0003 D	  44 
	RET			; 0004 .	  c9 

