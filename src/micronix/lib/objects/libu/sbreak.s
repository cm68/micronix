;
;	disas version 3
;	sbreak.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 7 text: 0(107) data: 107(2) bss: 0
;
;	text	0000	0107
;	data	0107	0002
;	undef	0200	0004
;
c.ret	equ	0200h
c.ent	equ	0201h
__memory	equ	0202h
__break	equ	0203h
;

	org	0000H
_sbreak: CALL	_sbreak		; 0000 ...	  cd 00 00 
	PUSH	AF		; 0003 .	  f5 
H0004: 	PUSH	AF		; 0004 .	  f5 
	PUSH	AF		; 0005 .	  f5 
	PUSH	AF		; 0006 .	  f5 
	LD	HL,FFF8H	; 0007 !..	  21 f8 ff 
	ADD	HL,DE		; 000a .	  19 
	PUSH	HL		; 000b .	  e5 
	LD	HL,H0004	; 000c !..	  21 04 00 
	ADD	HL,DE		; 000f .	  19 
	LD	C,(HL)		; 0010 N	  4e 
	INC	HL		; 0011 #	  23 
	LD	B,(HL)		; 0012 F	  46 
	PUSH	BC		; 0013 .	  c5 
	CALL	_sbrk		; 0014 .B.	  cd 42 00 
	POP	AF		; 0017 .	  f1 
	POP	HL		; 0018 .	  e1 
	LD	A,C		; 0019 y	  79 
	LD	(HL),A		; 001a w	  77 
	LD	A,B		; 001b x	  78 
	INC	HL		; 001c #	  23 
	LD	(HL),A		; 001d w	  77 
	LD	HL,FFF8H	; 001e !..	  21 f8 ff 
	ADD	HL,DE		; 0021 .	  19 
	LD	A,(HL)		; 0022 ~	  7e 
	CP	FFH		; 0023 ..	  fe ff 
	JP	NZ,H002c	; 0025 .,.	  c2 2c 00 
	INC	HL		; 0028 #	  23 
	LD	A,(HL)		; 0029 ~	  7e 
	CP	FFH		; 002a ..	  fe ff 
H002c: 	JP	NZ,H0035	; 002c .5.	  c2 35 00 
	LD	BC,_sbreak	; 002f ...	  01 00 00 
	JP	_sbreak		; 0032 ...	  c3 00 00 

H0035: 	LD	HL,FFF8H	; 0035 !..	  21 f8 ff 
	ADD	HL,DE		; 0038 .	  19 
	LD	A,(HL)		; 0039 ~	  7e 
	INC	HL		; 003a #	  23 
	LD	H,(HL)		; 003b f	  66 
	LD	L,A		; 003c o	  6f 
	LD	C,L		; 003d M	  4d 
	LD	B,H		; 003e D	  44 
	JP	_sbreak		; 003f ...	  c3 00 00 

_sbrk: 	CALL	_sbreak		; 0042 ...	  cd 00 00 
	LD	HL,FFF6H	; 0045 !..	  21 f6 ff 
	ADD	HL,SP		; 0048 9	  39 
	LD	SP,HL		; 0049 .	  f9 
	LD	HL,FFF8H	; 004a !..	  21 f8 ff 
	ADD	HL,DE		; 004d .	  19 
	LD	A,(H0107)	; 004e :..	  3a 07 01 
	LD	(HL),A		; 0051 w	  77 
	LD	A,(H0108)	; 0052 :..	  3a 08 01 
	INC	HL		; 0055 #	  23 
	LD	(HL),A		; 0056 w	  77 
	LD	HL,FFF6H	; 0057 !..	  21 f6 ff 
	ADD	HL,DE		; 005a .	  19 
	PUSH	HL		; 005b .	  e5 
	LD	HL,FFF8H	; 005c !..	  21 f8 ff 
	ADD	HL,DE		; 005f .	  19 
	LD	A,(HL)		; 0060 ~	  7e 
	INC	HL		; 0061 #	  23 
	LD	H,(HL)		; 0062 f	  66 
	LD	L,A		; 0063 o	  6f 
	PUSH	HL		; 0064 .	  e5 
	LD	HL,H0004	; 0065 !..	  21 04 00 
	ADD	HL,DE		; 0068 .	  19 
	LD	A,(HL)		; 0069 ~	  7e 
	INC	HL		; 006a #	  23 
	LD	H,(HL)		; 006b f	  66 
	LD	L,A		; 006c o	  6f 
	EX	(SP),HL		; 006d .	  e3 
	POP	BC		; 006e .	  c1 
	ADD	HL,BC		; 006f .	  09 
	POP	BC		; 0070 .	  c1 
	LD	A,L		; 0071 }	  7d 
	LD	(BC),A		; 0072 .	  02 
	LD	A,H		; 0073 |	  7c 
	INC	BC		; 0074 .	  03 
	LD	(BC),A		; 0075 .	  02 
	LD	HL,FFF6H	; 0076 !..	  21 f6 ff 
	ADD	HL,DE		; 0079 .	  19 
	LD	C,(HL)		; 007a N	  4e 
	INC	HL		; 007b #	  23 
	LD	B,(HL)		; 007c F	  46 
	PUSH	BC		; 007d .	  c5 
	CALL	_sbreak		; 007e ...	  cd 00 00 
	POP	AF		; 0081 .	  f1 
	LD	A,C		; 0082 y	  79 
	CP	FFH		; 0083 ..	  fe ff 
	JP	NZ,H008b	; 0085 ...	  c2 8b 00 
	LD	A,B		; 0088 x	  78 
	CP	FFH		; 0089 ..	  fe ff 
H008b: 	JP	NZ,H0094	; 008b ...	  c2 94 00 
	LD	BC,FFFFH	; 008e ...	  01 ff ff 
	JP	_sbreak		; 0091 ...	  c3 00 00 

H0094: 	LD	HL,FFF6H	; 0094 !..	  21 f6 ff 
	ADD	HL,DE		; 0097 .	  19 
	LD	A,(HL)		; 0098 ~	  7e 
	INC	HL		; 0099 #	  23 
	LD	H,(HL)		; 009a f	  66 
	LD	L,A		; 009b o	  6f 
	LD	(H0107),HL	; 009c "..	  22 07 01 
	LD	HL,FFF8H	; 009f !..	  21 f8 ff 
	ADD	HL,DE		; 00a2 .	  19 
	LD	A,(HL)		; 00a3 ~	  7e 
	INC	HL		; 00a4 #	  23 
	LD	H,(HL)		; 00a5 f	  66 
	LD	L,A		; 00a6 o	  6f 
	LD	C,L		; 00a7 M	  4d 
	LD	B,H		; 00a8 D	  44 
	JP	_sbreak		; 00a9 ...	  c3 00 00 

	DB	CDH,00H,00H,"!",F6H,FFH,"9",F9H,"!",F8H,FFH,19H
	DB	":",07H,01H,"w:",08H,01H,"#w!",F6H,FFH,19H,E5H
	DB	"!",04H,00H,19H,C1H,"~",02H,"#~",03H,02H,"!",F6H
	DB	FFH,19H,"N#F",C5H,CDH,00H,00H,F1H,"y",FEH,FFH
	DB	C2H,E6H,00H,"x",FEH,FFH,C2H,EFH,00H,01H,FFH,FFH
	DB	C3H,00H,00H,"!",F6H,FFH,19H,"~#fo",22H,07H,01H
	DB	"!",F8H,FFH,19H,"~#foMD",C3H,00H,00H

	org	0107H
H0107: 	DW	_sbreak
