;
;	disas version 3
;	chdir.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(18) data: 18(4) bss: 0
;
;	text	0000	0018
;	data	0018	0004
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_chdir: LD	HL,H0002	; 0000 !..	  21 02 00 
	ADD	HL,SP		; 0003 9	  39 
	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	LD	(H001a),HL	; 0008 "..	  22 1a 00 
	SYS	indir 18 00 	; 000b ....	  cf 00 18 00 
	LD	BC,_chdir	; 000f ...	  01 00 00 
	RET	NC		; 0012 .	  d0 
	DEC	BC		; 0013 .	  0b 
	LD	(_chdir),HL	; 0014 "..	  22 00 00 
	RET			; 0017 .	  c9 


	org	0018H
	DB	CFH,0CH
H001a: 	DW	_chdir
