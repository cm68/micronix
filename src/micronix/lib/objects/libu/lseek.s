;
;	disas version 3
;	lseek.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 9 text: 0(f4) data: f4(4) bss: 0
;
;	text	0000	00f4
;	data	00f4	0004
;	undef	0100	0008
;
c.lmod	equ	0100h
c.r1	equ	0101h
c.r0	equ	0102h
c.ret	equ	0103h
c.ent	equ	0104h
c.lcpy	equ	0105h
_seek	equ	0106h
c.ldiv	equ	0107h
;

	org	0000H
_lseek: CALL	_lseek		; 0000 ...	  cd 00 00 
H0003: 	LD	HL,H000a	; 0003 !..	  21 0a 00 
H0006: 	ADD	HL,DE		; 0006 .	  19 
	LD	A,(HL)		; 0007 ~	  7e 
	SUB	03H		; 0008 ..	  d6 03 
H000a: 	INC	HL		; 000a #	  23 
	LD	A,(HL)		; 000b ~	  7e 
	SBC	A,00H		; 000c ..	  de 00 
	JP	P,H00b6		; 000e ...	  f2 b6 00 
	LD	HL,H0006	; 0011 !..	  21 06 00 
	ADD	HL,DE		; 0014 .	  19 
	PUSH	HL		; 0015 .	  e5 
	POP	HL		; 0016 .	  e1 
	LD	BC,_lseek	; 0017 ...	  01 00 00 
	CALL	_lseek		; 001a ...	  cd 00 00 
	LD	HL,_lseek	; 001d !..	  21 00 00 
	PUSH	HL		; 0020 .	  e5 
	SUB	A		; 0021 .	  97 
	LD	(_lseek),A	; 0022 2..	  32 00 00 
	LD	(H0001),A	; 0025 2..	  32 01 00 
	LD	(H0002),A	; 0028 2..	  32 02 00 
	LD	A,02H		; 002b >.	  3e 02 
	LD	(H0003),A	; 002d 2..	  32 03 00 
	LD	HL,_lseek	; 0030 !..	  21 00 00 
	PUSH	HL		; 0033 .	  e5 
	CALL	_lseek		; 0034 ...	  cd 00 00 
	POP	HL		; 0037 .	  e1 
	INC	HL		; 0038 #	  23 
	INC	HL		; 0039 #	  23 
	LD	A,(HL)		; 003a ~	  7e 
	LD	(H00f4),A	; 003b 2..	  32 f4 00 
	INC	HL		; 003e #	  23 
	LD	A,(HL)		; 003f ~	  7e 
	LD	(H00f5),A	; 0040 2..	  32 f5 00 
	LD	HL,H0006	; 0043 !..	  21 06 00 
	ADD	HL,DE		; 0046 .	  19 
	PUSH	HL		; 0047 .	  e5 
	POP	HL		; 0048 .	  e1 
	LD	BC,_lseek	; 0049 ...	  01 00 00 
	CALL	_lseek		; 004c ...	  cd 00 00 
	LD	HL,_lseek	; 004f !..	  21 00 00 
	PUSH	HL		; 0052 .	  e5 
	SUB	A		; 0053 .	  97 
	LD	(_lseek),A	; 0054 2..	  32 00 00 
	LD	(H0001),A	; 0057 2..	  32 01 00 
	LD	(H0002),A	; 005a 2..	  32 02 00 
	LD	A,02H		; 005d >.	  3e 02 
	LD	(H0003),A	; 005f 2..	  32 03 00 
	LD	HL,_lseek	; 0062 !..	  21 00 00 
	PUSH	HL		; 0065 .	  e5 
	CALL	_lseek		; 0066 ...	  cd 00 00 
	POP	HL		; 0069 .	  e1 
	INC	HL		; 006a #	  23 
	INC	HL		; 006b #	  23 
	LD	A,(HL)		; 006c ~	  7e 
	LD	(H00f6),A	; 006d 2..	  32 f6 00 
	INC	HL		; 0070 #	  23 
	LD	A,(HL)		; 0071 ~	  7e 
	LD	(H00f7),A	; 0072 2..	  32 f7 00 
	LD	HL,H000a	; 0075 !..	  21 0a 00 
	ADD	HL,DE		; 0078 .	  19 
	LD	A,(HL)		; 0079 ~	  7e 
	INC	HL		; 007a #	  23 
	LD	H,(HL)		; 007b f	  66 
	LD	L,A		; 007c o	  6f 
	INC	HL		; 007d #	  23 
	INC	HL		; 007e #	  23 
	INC	HL		; 007f #	  23 
	PUSH	HL		; 0080 .	  e5 
	LD	HL,(H00f4)	; 0081 *..	  2a f4 00 
	PUSH	HL		; 0084 .	  e5 
	LD	HL,H0004	; 0085 !..	  21 04 00 
	ADD	HL,DE		; 0088 .	  19 
	LD	C,(HL)		; 0089 N	  4e 
	INC	HL		; 008a #	  23 
	LD	B,(HL)		; 008b F	  46 
	PUSH	BC		; 008c .	  c5 
	CALL	_lseek		; 008d ...	  cd 00 00 
	POP	AF		; 0090 .	  f1 
	POP	AF		; 0091 .	  f1 
	POP	AF		; 0092 .	  f1 
	LD	A,B		; 0093 x	  78 
	OR	A		; 0094 .	  b7 
	JP	M,H00e1		; 0095 ...	  fa e1 00 
	LD	HL,H0001	; 0098 !..	  21 01 00 
	PUSH	HL		; 009b .	  e5 
	LD	HL,(H00f6)	; 009c *..	  2a f6 00 
	PUSH	HL		; 009f .	  e5 
	LD	HL,H0004	; 00a0 !..	  21 04 00 
	ADD	HL,DE		; 00a3 .	  19 
	LD	C,(HL)		; 00a4 N	  4e 
	INC	HL		; 00a5 #	  23 
	LD	B,(HL)		; 00a6 F	  46 
	PUSH	BC		; 00a7 .	  c5 
	CALL	_lseek		; 00a8 ...	  cd 00 00 
	POP	AF		; 00ab .	  f1 
	POP	AF		; 00ac .	  f1 
	POP	AF		; 00ad .	  f1 
	LD	A,B		; 00ae x	  78 
	OR	A		; 00af .	  b7 
	JP	P,H00e7		; 00b0 ...	  f2 e7 00 
	JP	H00e1		; 00b3 ...	  c3 e1 00 

H00b6: 	LD	HL,H000a	; 00b6 !..	  21 0a 00 
	ADD	HL,DE		; 00b9 .	  19 
	LD	C,(HL)		; 00ba N	  4e 
	INC	HL		; 00bb #	  23 
	LD	B,(HL)		; 00bc F	  46 
	PUSH	BC		; 00bd .	  c5 
	LD	HL,H0006	; 00be !..	  21 06 00 
	ADD	HL,DE		; 00c1 .	  19 
	INC	HL		; 00c2 #	  23 
	INC	HL		; 00c3 #	  23 
	LD	C,(HL)		; 00c4 N	  4e 
	INC	HL		; 00c5 #	  23 
	LD	B,(HL)		; 00c6 F	  46 
	PUSH	BC		; 00c7 .	  c5 
	LD	HL,H0004	; 00c8 !..	  21 04 00 
	ADD	HL,DE		; 00cb .	  19 
	LD	C,(HL)		; 00cc N	  4e 
	INC	HL		; 00cd #	  23 
	LD	B,(HL)		; 00ce F	  46 
	PUSH	BC		; 00cf .	  c5 
	CALL	_lseek		; 00d0 ...	  cd 00 00 
	POP	AF		; 00d3 .	  f1 
	POP	AF		; 00d4 .	  f1 
	POP	AF		; 00d5 .	  f1 
	LD	A,B		; 00d6 x	  78 
	OR	A		; 00d7 .	  b7 
	JP	P,H00e7		; 00d8 ...	  f2 e7 00 
	LD	BC,FFFFH	; 00db ...	  01 ff ff 
	JP	_lseek		; 00de ...	  c3 00 00 

H00e1: 	LD	BC,FFFFH	; 00e1 ...	  01 ff ff 
	JP	_lseek		; 00e4 ...	  c3 00 00 

H00e7: 	LD	HL,H0004	; 00e7 !..	  21 04 00 
	ADD	HL,DE		; 00ea .	  19 
	LD	A,(HL)		; 00eb ~	  7e 
	INC	HL		; 00ec #	  23 
	LD	H,(HL)		; 00ed f	  66 
	LD	L,A		; 00ee o	  6f 
	LD	C,L		; 00ef M	  4d 
	LD	B,H		; 00f0 D	  44 
	JP	_lseek		; 00f1 ...	  c3 00 00 


	org	00f4H
H00f4: 	DW	_lseek
H00f6: 	DW	_lseek
