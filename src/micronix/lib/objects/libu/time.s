;
;	disas version 3
;	time.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 2 text: 0(1f) data: 1f(0) bss: 0
;
;	text	0000	001f
;	undef	0100	0001
;
_errno	equ	0100h
;

	org	0000H
_time: 	PUSH	DE		; 0000 .	  d5 
	SYS	time 		; 0001 ..	  cf 0d 
	EX	DE,HL		; 0003 .	  eb 
	PUSH	HL		; 0004 .	  e5 
	LD	HL,H0006	; 0005 !..	  21 06 00 
	ADD	HL,SP		; 0008 9	  39 
	LD	A,(HL)		; 0009 ~	  7e 
	INC	HL		; 000a #	  23 
	LD	H,(HL)		; 000b f	  66 
	LD	L,A		; 000c o	  6f 
	LD	(HL),E		; 000d s	  73 
	INC	HL		; 000e #	  23 
	LD	(HL),D		; 000f r	  72 
	INC	HL		; 0010 #	  23 
	POP	DE		; 0011 .	  d1 
	LD	(HL),E		; 0012 s	  73 
	INC	HL		; 0013 #	  23 
	LD	(HL),D		; 0014 r	  72 
	POP	DE		; 0015 .	  d1 
	LD	BC,_time	; 0016 ...	  01 00 00 
	RET	NC		; 0019 .	  d0 
	DEC	BC		; 001a .	  0b 
	LD	(_time),HL	; 001b "..	  22 00 00 
	RET			; 001e .	  c9 

