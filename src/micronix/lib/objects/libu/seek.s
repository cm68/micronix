;
;	disas version 3
;	seek.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(2b) data: 2b(6) bss: 0
;
;	text	0000	002b
;	data	002b	0006
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_seek: 	LD	HL,H0004	; 0000 !..	  21 04 00 
	ADD	HL,SP		; 0003 9	  39 
H0004: 	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
H0006: 	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	LD	(H002d),HL	; 0008 "-.	  22 2d 00 
	LD	HL,H0006	; 000b !..	  21 06 00 
	ADD	HL,SP		; 000e 9	  39 
	LD	A,(HL)		; 000f ~	  7e 
	INC	HL		; 0010 #	  23 
	LD	H,(HL)		; 0011 f	  66 
	LD	L,A		; 0012 o	  6f 
	LD	(H002f),HL	; 0013 "/.	  22 2f 00 
	LD	HL,H0002	; 0016 !..	  21 02 00 
	ADD	HL,SP		; 0019 9	  39 
	LD	A,(HL)		; 001a ~	  7e 
	INC	HL		; 001b #	  23 
	LD	H,(HL)		; 001c f	  66 
	LD	L,A		; 001d o	  6f 
	SYS	indir 2b 00 	; 001e ..+.	  cf 00 2b 00 
	LD	BC,_seek	; 0022 ...	  01 00 00 
	RET	NC		; 0025 .	  d0 
	DEC	BC		; 0026 .	  0b 
	LD	(_seek),HL	; 0027 "..	  22 00 00 
	RET			; 002a .	  c9 


	org	002bH
	DB	CFH,13H
H002d: 	DW	_seek
H002f: 	DW	_seek
