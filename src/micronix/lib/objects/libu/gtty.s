;
;	disas version 3
;	gtty.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(20) data: 20(4) bss: 0
;
;	text	0000	0020
;	data	0020	0004
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_gtty: 	LD	HL,H0004	; 0000 !..	  21 04 00 
	ADD	HL,SP		; 0003 9	  39 
H0004: 	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	LD	(H0022),HL	; 0008 "".	  22 22 00 
	LD	HL,H0002	; 000b !..	  21 02 00 
	ADD	HL,SP		; 000e 9	  39 
	LD	A,(HL)		; 000f ~	  7e 
	INC	HL		; 0010 #	  23 
	LD	H,(HL)		; 0011 f	  66 
	LD	L,A		; 0012 o	  6f 
	SYS	indir 20 00 	; 0013 ....	  cf 00 20 00 
	LD	BC,_gtty	; 0017 ...	  01 00 00 
	RET	NC		; 001a .	  d0 
	DEC	BC		; 001b .	  0b 
	LD	(_gtty),HL	; 001c "..	  22 00 00 
	RET			; 001f .	  c9 


	org	0020H
	DB	CFH," "
H0022: 	DW	_gtty
