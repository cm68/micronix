;
;	disas version 3
;	signal.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 8 text: 0(eb) data: eb(2) bss: 0
;
;	text	0000	00eb
;	data	00eb	0002
;	undef	0100	0007
;
__signal	equ	0100h
c.r4	equ	0101h
c.r2	equ	0102h
__stab	equ	0103h
__jtab	equ	0104h
c.rets	equ	0105h
c.ents	equ	0106h
;

	org	0000H
_signal: CALL	_signal		; 0000 ...	  cd 00 00 
	LD	HL,H0004	; 0003 !..	  21 04 00 
H0006: 	ADD	HL,DE		; 0006 .	  19 
	LD	A,(HL)		; 0007 ~	  7e 
	INC	HL		; 0008 #	  23 
	LD	H,(HL)		; 0009 f	  66 
	LD	L,A		; 000a o	  6f 
	LD	(_signal),HL	; 000b "..	  22 00 00 
	LD	HL,H0006	; 000e !..	  21 06 00 
	ADD	HL,DE		; 0011 .	  19 
	LD	A,(HL)		; 0012 ~	  7e 
	INC	HL		; 0013 #	  23 
	LD	H,(HL)		; 0014 f	  66 
	LD	L,A		; 0015 o	  6f 
	LD	(_signal),HL	; 0016 "..	  22 00 00 
	LD	A,(_signal)	; 0019 :..	  3a 00 00 
	SUB	01H		; 001c ..	  d6 01 
	LD	A,(H0001)	; 001e :..	  3a 01 00 
	SBC	A,00H		; 0021 ..	  de 00 
	JP	M,H0033		; 0023 .3.	  fa 33 00 
	LD	HL,_signal	; 0026 !..	  21 00 00 
	LD	A,0FH		; 0029 >.	  3e 0f 
	SUB	(HL)		; 002b .	  96 
	LD	A,00H		; 002c >.	  3e 00 
	INC	HL		; 002e #	  23 
	SBC	A,(HL)		; 002f .	  9e 
	JP	P,H0039		; 0030 .9.	  f2 39 00 
H0033: 	LD	BC,FFFFH	; 0033 ...	  01 ff ff 
	JP	_signal		; 0036 ...	  c3 00 00 

H0039: 	LD	A,(_signal)	; 0039 :..	  3a 00 00 
	CP	01H		; 003c ..	  fe 01 
	JP	NZ,H0046	; 003e .F.	  c2 46 00 
	LD	A,(H0001)	; 0041 :..	  3a 01 00 
	CP	00H		; 0044 ..	  fe 00 
H0046: 	JP	Z,H0058		; 0046 .X.	  ca 58 00 
	LD	HL,_signal	; 0049 !..	  21 00 00 
	LD	A,(HL)		; 004c ~	  7e 
	INC	HL		; 004d #	  23 
	OR	(HL)		; 004e .	  b6 
	JP	Z,H0058		; 004f .X.	  ca 58 00 
	LD	BC,H0001	; 0052 ...	  01 01 00 
	JP	H005d		; 0055 .].	  c3 5d 00 

H0058: 	LD	HL,(_signal)	; 0058 *..	  2a 00 00 
	LD	C,L		; 005b M	  4d 
	LD	B,H		; 005c D	  44 
H005d: 	PUSH	BC		; 005d .	  c5 
	LD	HL,(_signal)	; 005e *..	  2a 00 00 
	PUSH	HL		; 0061 .	  e5 
	CALL	_signal		; 0062 ...	  cd 00 00 
	POP	AF		; 0065 .	  f1 
	POP	AF		; 0066 .	  f1 
	LD	L,C		; 0067 i	  69 
	LD	H,B		; 0068 `	  60 
	LD	(H00eb),HL	; 0069 "..	  22 eb 00 
	LD	A,(H00eb)	; 006c :..	  3a eb 00 
	CP	01H		; 006f ..	  fe 01 
	JP	NZ,H0079	; 0071 .y.	  c2 79 00 
	LD	A,(H00ec)	; 0074 :..	  3a ec 00 
	CP	00H		; 0077 ..	  fe 00 
H0079: 	JP	Z,H0098		; 0079 ...	  ca 98 00 
	LD	HL,H00eb	; 007c !..	  21 eb 00 
	LD	A,(HL)		; 007f ~	  7e 
	INC	HL		; 0080 #	  23 
	OR	(HL)		; 0081 .	  b6 
	JP	Z,H0098		; 0082 ...	  ca 98 00 
	LD	HL,(_signal)	; 0085 *..	  2a 00 00 
	LD	BC,FFFFH	; 0088 ...	  01 ff ff 
	ADD	HL,BC		; 008b .	  09 
	ADD	HL,HL		; 008c )	  29 
	LD	BC,_signal	; 008d ...	  01 00 00 
	ADD	HL,BC		; 0090 .	  09 
	LD	A,(HL)		; 0091 ~	  7e 
	INC	HL		; 0092 #	  23 
	LD	H,(HL)		; 0093 f	  66 
	LD	L,A		; 0094 o	  6f 
	LD	(H00eb),HL	; 0095 "..	  22 eb 00 
H0098: 	LD	A,(_signal)	; 0098 :..	  3a 00 00 
	CP	01H		; 009b ..	  fe 01 
	JP	NZ,H00a5	; 009d ...	  c2 a5 00 
	LD	A,(H0001)	; 00a0 :..	  3a 01 00 
	CP	00H		; 00a3 ..	  fe 00 
H00a5: 	JP	Z,H00e3		; 00a5 ...	  ca e3 00 
	LD	HL,_signal	; 00a8 !..	  21 00 00 
	LD	A,(HL)		; 00ab ~	  7e 
	INC	HL		; 00ac #	  23 
	OR	(HL)		; 00ad .	  b6 
	JP	Z,H00e3		; 00ae ...	  ca e3 00 
	LD	HL,(_signal)	; 00b1 *..	  2a 00 00 
	LD	BC,FFFFH	; 00b4 ...	  01 ff ff 
	ADD	HL,BC		; 00b7 .	  09 
	ADD	HL,HL		; 00b8 )	  29 
	LD	BC,_signal	; 00b9 ...	  01 00 00 
	ADD	HL,BC		; 00bc .	  09 
	LD	A,(_signal)	; 00bd :..	  3a 00 00 
	LD	(HL),A		; 00c0 w	  77 
	LD	A,(H0001)	; 00c1 :..	  3a 01 00 
	INC	HL		; 00c4 #	  23 
	LD	(HL),A		; 00c5 w	  77 
	LD	HL,(_signal)	; 00c6 *..	  2a 00 00 
	LD	BC,FFFFH	; 00c9 ...	  01 ff ff 
	ADD	HL,BC		; 00cc .	  09 
	PUSH	BC		; 00cd .	  c5 
	LD	C,L		; 00ce M	  4d 
	LD	B,H		; 00cf D	  44 
	ADD	HL,HL		; 00d0 )	  29 
	ADD	HL,BC		; 00d1 .	  09 
	ADD	HL,HL		; 00d2 )	  29 
	ADD	HL,BC		; 00d3 .	  09 
	POP	BC		; 00d4 .	  c1 
	LD	BC,_signal	; 00d5 ...	  01 00 00 
	ADD	HL,BC		; 00d8 .	  09 
	PUSH	HL		; 00d9 .	  e5 
	LD	HL,(_signal)	; 00da *..	  2a 00 00 
	PUSH	HL		; 00dd .	  e5 
	CALL	_signal		; 00de ...	  cd 00 00 
	POP	AF		; 00e1 .	  f1 
	POP	AF		; 00e2 .	  f1 
H00e3: 	LD	HL,(H00eb)	; 00e3 *..	  2a eb 00 
	LD	C,L		; 00e6 M	  4d 
	LD	B,H		; 00e7 D	  44 
	JP	_signal		; 00e8 ...	  c3 00 00 


	org	00ebH
H00eb: 	DW	_signal
