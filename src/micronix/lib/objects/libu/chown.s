;
;	disas version 3
;	chown.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(23) data: 23(6) bss: 0
;
;	text	0000	0023
;	data	0023	0006
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_chown: LD	HL,H0002	; 0000 !..	  21 02 00 
	ADD	HL,SP		; 0003 9	  39 
H0004: 	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	LD	(H0025),HL	; 0008 "%.	  22 25 00 
	LD	HL,H0004	; 000b !..	  21 04 00 
	ADD	HL,SP		; 000e 9	  39 
	LD	A,(HL)		; 000f ~	  7e 
	INC	HL		; 0010 #	  23 
	LD	H,(HL)		; 0011 f	  66 
	LD	L,A		; 0012 o	  6f 
	LD	(H0027),HL	; 0013 "'.	  22 27 00 
	SYS	indir 23 00 	; 0016 ..#.	  cf 00 23 00 
	LD	BC,_chown	; 001a ...	  01 00 00 
	RET	NC		; 001d .	  d0 
	DEC	BC		; 001e .	  0b 
	LD	(_chown),HL	; 001f "..	  22 00 00 
	RET			; 0022 .	  c9 


	org	0023H
	DB	CFH,10H
H0025: 	DW	_chown
H0027: 	DW	_chown
