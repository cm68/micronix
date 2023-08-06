;
;	disas version 3
;	mknod.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(2e) data: 2e(8) bss: 0
;
;	text	0000	002e
;	data	002e	0008
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_mknod: LD	HL,H0002	; 0000 !..	  21 02 00 
	ADD	HL,SP		; 0003 9	  39 
H0004: 	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
H0006: 	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	LD	(H0030),HL	; 0008 "0.	  22 30 00 
	LD	HL,H0004	; 000b !..	  21 04 00 
	ADD	HL,SP		; 000e 9	  39 
	LD	A,(HL)		; 000f ~	  7e 
	INC	HL		; 0010 #	  23 
	LD	H,(HL)		; 0011 f	  66 
	LD	L,A		; 0012 o	  6f 
	LD	(H0032),HL	; 0013 "2.	  22 32 00 
	LD	HL,H0006	; 0016 !..	  21 06 00 
	ADD	HL,SP		; 0019 9	  39 
	LD	A,(HL)		; 001a ~	  7e 
	INC	HL		; 001b #	  23 
	LD	H,(HL)		; 001c f	  66 
	LD	L,A		; 001d o	  6f 
	LD	(H0034),HL	; 001e "4.	  22 34 00 
	SYS	indir 2e 00 	; 0021 ....	  cf 00 2e 00 
	LD	BC,_mknod	; 0025 ...	  01 00 00 
	RET	NC		; 0028 .	  d0 
	DEC	BC		; 0029 .	  0b 
	LD	(_mknod),HL	; 002a "..	  22 00 00 
	RET			; 002d .	  c9 


	org	002eH
	DB	CFH,0EH
H0030: 	DW	_mknod
H0032: 	DW	_mknod
H0034: 	DW	_mknod
