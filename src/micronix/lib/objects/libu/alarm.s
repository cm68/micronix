;
;	disas version 3
;	alarm.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(10) data: 10(2) bss: 0
;
;	text	0000	0010
;	data	0010	0002
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_alarm: LD	HL,H0002	; 0000 !..	  21 02 00 
	ADD	HL,SP		; 0003 9	  39 
	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	SYS	indir 10 00 	; 0008 ....	  cf 00 10 00 
	LD	BC,_alarm	; 000c ...	  01 00 00 
	RET			; 000f .	  c9 


	org	0010H
	DB	CFH,1BH
