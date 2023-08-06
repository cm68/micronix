;
;	disas version 3
;	exec.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(21) data: 21(6) bss: 0
;
;	text	0000	0021
;	data	0021	0006
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_exec: 	LD	HL,H0002	; 0000 !..	  21 02 00 
	ADD	HL,SP		; 0003 9	  39 
H0004: 	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	LD	(H0023),HL	; 0008 "#.	  22 23 00 
	LD	HL,H0004	; 000b !..	  21 04 00 
	ADD	HL,SP		; 000e 9	  39 
	LD	A,(HL)		; 000f ~	  7e 
	INC	HL		; 0010 #	  23 
	LD	H,(HL)		; 0011 f	  66 
	LD	L,A		; 0012 o	  6f 
	LD	(H0025),HL	; 0013 "%.	  22 25 00 
	SYS	indir 21 00 	; 0016 ..!.	  cf 00 21 00 
	LD	BC,FFFFH	; 001a ...	  01 ff ff 
	LD	(_exec),HL	; 001d "..	  22 00 00 
	RET			; 0020 .	  c9 


	org	0021H
	DB	CFH,0BH
H0023: 	DW	_exec
H0025: 	DW	_exec
