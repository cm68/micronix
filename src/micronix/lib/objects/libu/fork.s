;
;	disas version 3
;	fork.o
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
_fork: 	SYS	fork 		; 0000 ..	  cf 02 
	JP	H000f		; 0002 ...	  c3 0f 00 

	DB	"MD",D0H,01H,FFH,FFH,22H,00H,00H,C9H
H000f: 	LD	BC,_fork	; 000f ...	  01 00 00 
	RET			; 0012 .	  c9 

