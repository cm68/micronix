;
;	disas version 3
;	create.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 6 text: 0(b5) data: b5(0) bss: 0
;
;	text	0000	00b5
;	undef	0100	0005
;
_close	equ	0100h
_creat	equ	0101h
c.ret	equ	0102h
c.ent	equ	0103h
_open	equ	0104h
;

	org	0000H
_create: CALL	_create		; 0000 ...	  cd 00 00 
	PUSH	AF		; 0003 .	  f5 
H0004: 	PUSH	AF		; 0004 .	  f5 
	PUSH	AF		; 0005 .	  f5 
H0006: 	PUSH	AF		; 0006 .	  f5 
	LD	HL,FFF8H	; 0007 !..	  21 f8 ff 
	ADD	HL,DE		; 000a .	  19 
	PUSH	HL		; 000b .	  e5 
	LD	HL,01FFH	; 000c !..	  21 ff 01 
	PUSH	HL		; 000f .	  e5 
	LD	HL,H0004	; 0010 !..	  21 04 00 
	ADD	HL,DE		; 0013 .	  19 
	LD	C,(HL)		; 0014 N	  4e 
	INC	HL		; 0015 #	  23 
	LD	B,(HL)		; 0016 F	  46 
	PUSH	BC		; 0017 .	  c5 
	CALL	_create		; 0018 ...	  cd 00 00 
	POP	AF		; 001b .	  f1 
	POP	AF		; 001c .	  f1 
	POP	HL		; 001d .	  e1 
	LD	A,C		; 001e y	  79 
	LD	(HL),A		; 001f w	  77 
	LD	A,B		; 0020 x	  78 
	INC	HL		; 0021 #	  23 
	LD	(HL),A		; 0022 w	  77 
	LD	HL,FFF8H	; 0023 !..	  21 f8 ff 
	ADD	HL,DE		; 0026 .	  19 
	INC	HL		; 0027 #	  23 
	LD	A,(HL)		; 0028 ~	  7e 
	OR	A		; 0029 .	  b7 
	JP	P,H003a		; 002a .:.	  f2 3a 00 
	LD	HL,FFF8H	; 002d !..	  21 f8 ff 
	ADD	HL,DE		; 0030 .	  19 
	LD	A,(HL)		; 0031 ~	  7e 
	INC	HL		; 0032 #	  23 
	LD	H,(HL)		; 0033 f	  66 
	LD	L,A		; 0034 o	  6f 
	LD	C,L		; 0035 M	  4d 
	LD	B,H		; 0036 D	  44 
	JP	_create		; 0037 ...	  c3 00 00 

H003a: 	LD	HL,H0006	; 003a !..	  21 06 00 
	ADD	HL,DE		; 003d .	  19 
	LD	A,(HL)		; 003e ~	  7e 
	INC	HL		; 003f #	  23 
	OR	(HL)		; 0040 .	  b6 
	JP	NZ,H006f	; 0041 .o.	  c2 6f 00 
	LD	HL,FFF8H	; 0044 !..	  21 f8 ff 
	ADD	HL,DE		; 0047 .	  19 
	LD	C,(HL)		; 0048 N	  4e 
	INC	HL		; 0049 #	  23 
	LD	B,(HL)		; 004a F	  46 
	PUSH	BC		; 004b .	  c5 
	CALL	_create		; 004c ...	  cd 00 00 
	POP	AF		; 004f .	  f1 
	LD	HL,FFF8H	; 0050 !..	  21 f8 ff 
	ADD	HL,DE		; 0053 .	  19 
	PUSH	HL		; 0054 .	  e5 
	LD	HL,_create	; 0055 !..	  21 00 00 
	PUSH	HL		; 0058 .	  e5 
	LD	HL,H0004	; 0059 !..	  21 04 00 
	ADD	HL,DE		; 005c .	  19 
	LD	C,(HL)		; 005d N	  4e 
	INC	HL		; 005e #	  23 
	LD	B,(HL)		; 005f F	  46 
	PUSH	BC		; 0060 .	  c5 
	CALL	_create		; 0061 ...	  cd 00 00 
	POP	AF		; 0064 .	  f1 
	POP	AF		; 0065 .	  f1 
	POP	HL		; 0066 .	  e1 
	LD	A,C		; 0067 y	  79 
	LD	(HL),A		; 0068 w	  77 
	LD	A,B		; 0069 x	  78 
	INC	HL		; 006a #	  23 
	LD	(HL),A		; 006b w	  77 
	JP	H00a8		; 006c ...	  c3 a8 00 

H006f: 	LD	HL,H0006	; 006f !..	  21 06 00 
	ADD	HL,DE		; 0072 .	  19 
	LD	A,(HL)		; 0073 ~	  7e 
	CP	02H		; 0074 ..	  fe 02 
	JP	NZ,H007d	; 0076 .}.	  c2 7d 00 
	INC	HL		; 0079 #	  23 
	LD	A,(HL)		; 007a ~	  7e 
	CP	00H		; 007b ..	  fe 00 
H007d: 	JP	NZ,H00a8	; 007d ...	  c2 a8 00 
	LD	HL,FFF8H	; 0080 !..	  21 f8 ff 
	ADD	HL,DE		; 0083 .	  19 
	LD	C,(HL)		; 0084 N	  4e 
	INC	HL		; 0085 #	  23 
	LD	B,(HL)		; 0086 F	  46 
	PUSH	BC		; 0087 .	  c5 
	CALL	_create		; 0088 ...	  cd 00 00 
	POP	AF		; 008b .	  f1 
	LD	HL,FFF8H	; 008c !..	  21 f8 ff 
	ADD	HL,DE		; 008f .	  19 
	PUSH	HL		; 0090 .	  e5 
	LD	HL,H0002	; 0091 !..	  21 02 00 
	PUSH	HL		; 0094 .	  e5 
	LD	HL,H0004	; 0095 !..	  21 04 00 
	ADD	HL,DE		; 0098 .	  19 
	LD	C,(HL)		; 0099 N	  4e 
	INC	HL		; 009a #	  23 
	LD	B,(HL)		; 009b F	  46 
	PUSH	BC		; 009c .	  c5 
	CALL	_create		; 009d ...	  cd 00 00 
	POP	AF		; 00a0 .	  f1 
	POP	AF		; 00a1 .	  f1 
	POP	HL		; 00a2 .	  e1 
	LD	A,C		; 00a3 y	  79 
	LD	(HL),A		; 00a4 w	  77 
	LD	A,B		; 00a5 x	  78 
	INC	HL		; 00a6 #	  23 
	LD	(HL),A		; 00a7 w	  77 
H00a8: 	LD	HL,FFF8H	; 00a8 !..	  21 f8 ff 
	ADD	HL,DE		; 00ab .	  19 
	LD	A,(HL)		; 00ac ~	  7e 
	INC	HL		; 00ad #	  23 
	LD	H,(HL)		; 00ae f	  66 
	LD	L,A		; 00af o	  6f 
	LD	C,L		; 00b0 M	  4d 
	LD	B,H		; 00b1 D	  44 
	JP	_create		; 00b2 ...	  c3 00 00 

