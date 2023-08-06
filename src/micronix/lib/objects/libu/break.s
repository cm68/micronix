;
;	disas version 3
;	break.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(14) data: 14(4) bss: 0
;
;	text	0000	0014
;	data	0014	0004
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
__break: POP	BC		; 0000 .	  c1 
	POP	HL		; 0001 .	  e1 
	PUSH	HL		; 0002 .	  e5 
	PUSH	BC		; 0003 .	  c5 
	LD	(H0016),HL	; 0004 "..	  22 16 00 
	SYS	indir 14 00 	; 0007 ....	  cf 00 14 00 
	LD	BC,__break	; 000b ...	  01 00 00 
	RET	NC		; 000e .	  d0 
	DEC	BC		; 000f .	  0b 
	LD	(__break),HL	; 0010 "..	  22 00 00 
	RET			; 0013 .	  c9 


	org	0014H
	DB	CFH,11H
H0016: 	DW	__break
