;
;	disas version 3
;	pause.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(8) data: 8(2) bss: 0
;
;	text	0000	0008
;	data	0008	0002
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_pause: SYS	indir 08 00 	; 0000 ....	  cf 00 08 00 
	LD	BC,_pause	; 0004 ...	  01 00 00 
	RET			; 0007 .	  c9 


	org	0008H
	DB	CFH,1DH
