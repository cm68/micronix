;
;	disas version 3
;	uname.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 10 text: 0(af) data: af(16) bss: 0
;
;	text	0000	00af
;	data	00af	0016
;	undef	0100	0009
;
c.r0	equ	0100h
c.ulmod	equ	0101h
c.ret	equ	0102h
_getpid	equ	0103h
c.ent	equ	0104h
_ltob	equ	0105h
_time	equ	0106h
_itob	equ	0107h
_cpystr	equ	0108h
;

	org	0000H
H0000: 	CPL			; 0000 /	  2f 
H0001: 	LD	(HL),H		; 0001 t	  74 
H0002: 	LD	L,L		; 0002 m	  6d 
H0003: 	LD	(HL),B		; 0003 p	  70 
	CPL			; 0004 /	  2f 
	NOP			; 0005 .	  00 
_uname: CALL	H0000		; 0006 ...	  cd 00 00 
	LD	HL,H00c1	; 0009 !..	  21 c1 00 
	LD	A,(HL)		; 000c ~	  7e 
	INC	HL		; 000d #	  23 
	OR	(HL)		; 000e .	  b6 
	INC	HL		; 000f #	  23 
	OR	(HL)		; 0010 .	  b6 
	INC	HL		; 0011 #	  23 
	OR	(HL)		; 0012 .	  b6 
	JP	NZ,H003e	; 0013 .>.	  c2 3e 00 
	LD	HL,H00c1	; 0016 !..	  21 c1 00 
	PUSH	HL		; 0019 .	  e5 
	CALL	H0000		; 001a ...	  cd 00 00 
	POP	AF		; 001d .	  f1 
	LD	HL,H00c1	; 001e !..	  21 c1 00 
	PUSH	HL		; 0021 .	  e5 
	LD	A,27H		; 0022 >'	  3e 27 
	ADD	A,A		; 0024 .	  87 
	SBC	A,A		; 0025 .	  9f 
	LD	(H0000),A	; 0026 2..	  32 00 00 
	LD	(H0001),A	; 0029 2..	  32 01 00 
	LD	A,27H		; 002c >'	  3e 27 
	LD	(H0003),A	; 002e 2..	  32 03 00 
	LD	A,10H		; 0031 >.	  3e 10 
	LD	(H0002),A	; 0033 2..	  32 02 00 
	LD	HL,H0000	; 0036 !..	  21 00 00 
	PUSH	HL		; 0039 .	  e5 
	CALL	H0000		; 003a ...	  cd 00 00 
	POP	AF		; 003d .	  f1 
H003e: 	LD	HL,H0000	; 003e !..	  21 00 00 
	PUSH	HL		; 0041 .	  e5 
	LD	HL,H0000	; 0042 !..	  21 00 00 
	PUSH	HL		; 0045 .	  e5 
	LD	HL,H00af	; 0046 !..	  21 af 00 
	PUSH	HL		; 0049 .	  e5 
	CALL	H0000		; 004a ...	  cd 00 00 
	POP	AF		; 004d .	  f1 
	POP	AF		; 004e .	  f1 
	POP	AF		; 004f .	  f1 
	LD	L,C		; 0050 i	  69 
	LD	H,B		; 0051 `	  60 
	LD	(H00bf),HL	; 0052 "..	  22 bf 00 
	LD	HL,H000a	; 0055 !..	  21 0a 00 
	PUSH	HL		; 0058 .	  e5 
	CALL	H0000		; 0059 ...	  cd 00 00 
	PUSH	BC		; 005c .	  c5 
	LD	HL,(H00bf)	; 005d *..	  2a bf 00 
	PUSH	HL		; 0060 .	  e5 
	CALL	H0000		; 0061 ...	  cd 00 00 
	POP	AF		; 0064 .	  f1 
	POP	AF		; 0065 .	  f1 
	POP	AF		; 0066 .	  f1 
	LD	HL,(H00bf)	; 0067 *..	  2a bf 00 
	ADD	HL,BC		; 006a .	  09 
	LD	(H00bf),HL	; 006b "..	  22 bf 00 
	LD	HL,(H00bf)	; 006e *..	  2a bf 00 
	PUSH	HL		; 0071 .	  e5 
	LD	HL,(H00bf)	; 0072 *..	  2a bf 00 
	INC	HL		; 0075 #	  23 
	LD	(H00bf),HL	; 0076 "..	  22 bf 00 
	POP	HL		; 0079 .	  e1 
	LD	(HL),2DH	; 007a 6-	  36 2d 
	LD	HL,H000a	; 007c !..	  21 0a 00 
	PUSH	HL		; 007f .	  e5 
	LD	HL,H00c1	; 0080 !..	  21 c1 00 
	INC	HL		; 0083 #	  23 
	INC	HL		; 0084 #	  23 
	LD	C,(HL)		; 0085 N	  4e 
	INC	HL		; 0086 #	  23 
	LD	B,(HL)		; 0087 F	  46 
	PUSH	BC		; 0088 .	  c5 
	DEC	HL		; 0089 +	  2b 
	DEC	HL		; 008a +	  2b 
	DEC	HL		; 008b +	  2b 
	LD	C,(HL)		; 008c N	  4e 
	INC	HL		; 008d #	  23 
	LD	B,(HL)		; 008e F	  46 
	PUSH	BC		; 008f .	  c5 
	LD	HL,(H00bf)	; 0090 *..	  2a bf 00 
	PUSH	HL		; 0093 .	  e5 
	CALL	H0000		; 0094 ...	  cd 00 00 
	POP	AF		; 0097 .	  f1 
	POP	AF		; 0098 .	  f1 
	POP	AF		; 0099 .	  f1 
	POP	AF		; 009a .	  f1 
	LD	HL,(H00bf)	; 009b *..	  2a bf 00 
	ADD	HL,BC		; 009e .	  09 
	LD	(H00bf),HL	; 009f "..	  22 bf 00 
	LD	HL,(H00bf)	; 00a2 *..	  2a bf 00 
	LD	(HL),00H	; 00a5 6.	  36 00 
	LD	HL,H00af	; 00a7 !..	  21 af 00 
	LD	C,L		; 00aa M	  4d 
	LD	B,H		; 00ab D	  44 
	JP	H0000		; 00ac ...	  c3 00 00 


	org	00afH
H00af: 	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H
H00bf: 	DW	H0000
H00c1: 	DB	00H,00H,00H,00H
