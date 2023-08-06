;
;	disas version 3
;	./libutil/logmsg.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 8 text: 0(9d) data: 9d(2) bss: 0
;
;	text	0100	009d
;	data	019d	0002
;	undef	0200	0007
;
_strlen	equ	0200h
_creat	equ	0201h
c.ret	equ	0202h
c.ent	equ	0203h
_open	equ	0204h
_write	equ	0205h
_seek	equ	0206h
;

	org	0100H
H0100: 	DB	0AH,00H
H0102: 	DB	"logfile",00H
H010a: 	DB	"logfile",00H
_logmsg: CALL	c.ent		; 0112 ...	  cd 03 02 
	LD	A,(H019d)	; 0115 :..	  3a 9d 01 
	CP	FFH		; 0118 ..	  fe ff 
	JP	NZ,H0122	; 011a .".	  c2 22 01 
	LD	A,(H019e)	; 011d :..	  3a 9e 01 
	CP	FFH		; 0120 ..	  fe ff 
H0122: 	JP	NZ,H0150	; 0122 .P.	  c2 50 01 
	LD	HL,H0001	; 0125 !..	  21 01 00 
	PUSH	HL		; 0128 .	  e5 
	LD	HL,H010a	; 0129 !..	  21 0a 01 
	PUSH	HL		; 012c .	  e5 
	CALL	_open		; 012d ...	  cd 04 02 
	POP	AF		; 0130 .	  f1 
	POP	AF		; 0131 .	  f1 
	LD	L,C		; 0132 i	  69 
	LD	H,B		; 0133 `	  60 
	LD	(H019d),HL	; 0134 "..	  22 9d 01 
	LD	A,(H019e)	; 0137 :..	  3a 9e 01 
	OR	A		; 013a .	  b7 
	JP	P,H0150		; 013b .P.	  f2 50 01 
	LD	HL,01FFH	; 013e !..	  21 ff 01 
	PUSH	HL		; 0141 .	  e5 
	LD	HL,H0102	; 0142 !..	  21 02 01 
	PUSH	HL		; 0145 .	  e5 
	CALL	_creat		; 0146 ...	  cd 01 02 
	POP	AF		; 0149 .	  f1 
	POP	AF		; 014a .	  f1 
	LD	L,C		; 014b i	  69 
	LD	H,B		; 014c `	  60 
	LD	(H019d),HL	; 014d "..	  22 9d 01 
H0150: 	LD	A,(H019e)	; 0150 :..	  3a 9e 01 
	OR	A		; 0153 .	  b7 
	JP	M,H019a		; 0154 ...	  fa 9a 01 
	LD	HL,H0002	; 0157 !..	  21 02 00 
	PUSH	HL		; 015a .	  e5 
	LD	HL,H0000	; 015b !..	  21 00 00 
	PUSH	HL		; 015e .	  e5 
	LD	HL,(H019d)	; 015f *..	  2a 9d 01 
	PUSH	HL		; 0162 .	  e5 
	CALL	_seek		; 0163 ...	  cd 06 02 
	POP	AF		; 0166 .	  f1 
	POP	AF		; 0167 .	  f1 
	POP	AF		; 0168 .	  f1 
	LD	HL,H0004	; 0169 !..	  21 04 00 
	ADD	HL,DE		; 016c .	  19 
	LD	C,(HL)		; 016d N	  4e 
	INC	HL		; 016e #	  23 
	LD	B,(HL)		; 016f F	  46 
	PUSH	BC		; 0170 .	  c5 
	CALL	_strlen		; 0171 ...	  cd 00 02 
	POP	AF		; 0174 .	  f1 
	PUSH	BC		; 0175 .	  c5 
	LD	HL,H0004	; 0176 !..	  21 04 00 
	ADD	HL,DE		; 0179 .	  19 
	LD	C,(HL)		; 017a N	  4e 
	INC	HL		; 017b #	  23 
	LD	B,(HL)		; 017c F	  46 
	PUSH	BC		; 017d .	  c5 
	LD	HL,(H019d)	; 017e *..	  2a 9d 01 
	PUSH	HL		; 0181 .	  e5 
	CALL	_write		; 0182 ...	  cd 05 02 
	POP	AF		; 0185 .	  f1 
	POP	AF		; 0186 .	  f1 
	POP	AF		; 0187 .	  f1 
	LD	HL,H0001	; 0188 !..	  21 01 00 
	PUSH	HL		; 018b .	  e5 
	LD	HL,H0100	; 018c !..	  21 00 01 
	PUSH	HL		; 018f .	  e5 
	LD	HL,(H019d)	; 0190 *..	  2a 9d 01 
	PUSH	HL		; 0193 .	  e5 
	CALL	_write		; 0194 ...	  cd 05 02 
	POP	AF		; 0197 .	  f1 
	POP	AF		; 0198 .	  f1 
	POP	AF		; 0199 .	  f1 
H019a: 	JP	c.ret		; 019a ...	  c3 02 02 


	org	019dH
H019d: 	DW	FFFFH
