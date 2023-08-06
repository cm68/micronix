;
;	disas version 3
;	execv.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 5 text: 0(33) data: 33(0) bss: 0
;
;	text	0000	0033
;	undef	0100	0003
;
c.ret	equ	0100h
c.ent	equ	0101h
_exec	equ	0102h
;

	org	0000H
_execv: CALL	_execv		; 0000 ...	  cd 00 00 
	LD	HL,H0006	; 0003 !..	  21 06 00 
H0006: 	ADD	HL,DE		; 0006 .	  19 
	LD	C,(HL)		; 0007 N	  4e 
	INC	HL		; 0008 #	  23 
	LD	B,(HL)		; 0009 F	  46 
	PUSH	BC		; 000a .	  c5 
	LD	HL,H0004	; 000b !..	  21 04 00 
	ADD	HL,DE		; 000e .	  19 
	LD	C,(HL)		; 000f N	  4e 
	INC	HL		; 0010 #	  23 
	LD	B,(HL)		; 0011 F	  46 
	PUSH	BC		; 0012 .	  c5 
	CALL	_execv		; 0013 ...	  cd 00 00 
	POP	AF		; 0016 .	  f1 
	POP	AF		; 0017 .	  f1 
	JP	_execv		; 0018 ...	  c3 00 00 

_execl: CALL	_execv		; 001b ...	  cd 00 00 
	LD	HL,H0006	; 001e !..	  21 06 00 
	ADD	HL,DE		; 0021 .	  19 
	PUSH	HL		; 0022 .	  e5 
	LD	HL,H0004	; 0023 !..	  21 04 00 
	ADD	HL,DE		; 0026 .	  19 
	LD	C,(HL)		; 0027 N	  4e 
	INC	HL		; 0028 #	  23 
	LD	B,(HL)		; 0029 F	  46 
	PUSH	BC		; 002a .	  c5 
	CALL	_execv		; 002b ...	  cd 00 00 
	POP	AF		; 002e .	  f1 
	POP	AF		; 002f .	  f1 
	JP	_execv		; 0030 ...	  c3 00 00 

