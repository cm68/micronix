;
;	disas version 3
;	./libutil/hexdump.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 6 text: 0(218) data: 218(12) bss: 0
;
;	text	0100	0218
;	data	0318	0012
;	undef	0400	0004
;
c.imod	equ	0400h
_printf	equ	0401h
c.ret	equ	0402h
c.ent	equ	0403h
;

	org	0100H
H0100: 	DB	0AH,00H
H0102: 	DB	" ",00H
H0104: 	DEC	H		; 0104 %	  25 
	LD	H,E		; 0105 c	  63 
	NOP			; 0106 .	  00 
_dumpasc: CALL	c.ent		; 0107 ...	  cd 03 04 
	LD	HL,FFF7H	; 010a !..	  21 f7 ff 
	ADD	HL,SP		; 010d 9	  39 
	LD	SP,HL		; 010e .	  f9 
	LD	HL,FFF8H	; 010f !..	  21 f8 ff 
	ADD	HL,DE		; 0112 .	  19 
	SUB	A		; 0113 .	  97 
	LD	(HL),A		; 0114 w	  77 
	INC	HL		; 0115 #	  23 
	LD	(HL),A		; 0116 w	  77 
H0117: 	LD	HL,FFF8H	; 0117 !..	  21 f8 ff 
	ADD	HL,DE		; 011a .	  19 
	PUSH	HL		; 011b .	  e5 
	LD	HL,H0328	; 011c !(.	  21 28 03 
	POP	BC		; 011f .	  c1 
	LD	A,(BC)		; 0120 .	  0a 
	SUB	(HL)		; 0121 .	  96 
	INC	BC		; 0122 .	  03 
	LD	A,(BC)		; 0123 .	  0a 
	INC	HL		; 0124 #	  23 
	SBC	A,(HL)		; 0125 .	  9e 
	JP	P,H0157		; 0126 .W.	  f2 57 01 
	LD	HL,FFF7H	; 0129 !..	  21 f7 ff 
	ADD	HL,DE		; 012c .	  19 
	PUSH	HL		; 012d .	  e5 
	LD	HL,H0318	; 012e !..	  21 18 03 
	PUSH	HL		; 0131 .	  e5 
	LD	HL,FFF8H	; 0132 !..	  21 f8 ff 
	ADD	HL,DE		; 0135 .	  19 
	LD	A,(HL)		; 0136 ~	  7e 
	INC	HL		; 0137 #	  23 
	LD	H,(HL)		; 0138 f	  66 
	LD	L,A		; 0139 o	  6f 
	EX	(SP),HL		; 013a .	  e3 
	POP	BC		; 013b .	  c1 
	ADD	HL,BC		; 013c .	  09 
	POP	BC		; 013d .	  c1 
	LD	A,(HL)		; 013e ~	  7e 
	LD	(BC),A		; 013f .	  02 
	LD	HL,FFF7H	; 0140 !..	  21 f7 ff 
	ADD	HL,DE		; 0143 .	  19 
	LD	A,(HL)		; 0144 ~	  7e 
	CP	20H		; 0145 ..	  fe 20 
	JP	M,H0172		; 0147 .r.	  fa 72 01 
	LD	HL,FFF7H	; 014a !..	  21 f7 ff 
	ADD	HL,DE		; 014d .	  19 
	LD	A,(HL)		; 014e ~	  7e 
	CP	7FH		; 014f ..	  fe 7f 
	JP	M,H0178		; 0151 .x.	  fa 78 01 
	JP	H0172		; 0154 .r.	  c3 72 01 

H0157: 	LD	HL,H0100	; 0157 !..	  21 00 01 
	PUSH	HL		; 015a .	  e5 
	CALL	_printf		; 015b ...	  cd 01 04 
	POP	AF		; 015e .	  f1 
	JP	c.ret		; 015f ...	  c3 02 04 

H0162: 	LD	HL,FFF8H	; 0162 !..	  21 f8 ff 
	ADD	HL,DE		; 0165 .	  19 
	LD	A,(HL)		; 0166 ~	  7e 
	ADD	A,01H		; 0167 ..	  c6 01 
	LD	(HL),A		; 0169 w	  77 
	INC	HL		; 016a #	  23 
	LD	A,(HL)		; 016b ~	  7e 
	ADC	A,00H		; 016c ..	  ce 00 
	LD	(HL),A		; 016e w	  77 
	JP	H0117		; 016f ...	  c3 17 01 

H0172: 	LD	HL,FFF7H	; 0172 !..	  21 f7 ff 
	ADD	HL,DE		; 0175 .	  19 
	LD	(HL),2EH	; 0176 6.	  36 2e 
H0178: 	LD	HL,FFF7H	; 0178 !..	  21 f7 ff 
	ADD	HL,DE		; 017b .	  19 
	LD	A,(HL)		; 017c ~	  7e 
	LD	C,A		; 017d O	  4f 
	ADD	A,A		; 017e .	  87 
	SBC	A,A		; 017f .	  9f 
	LD	B,A		; 0180 G	  47 
	PUSH	BC		; 0181 .	  c5 
	LD	HL,H0104	; 0182 !..	  21 04 01 
	PUSH	HL		; 0185 .	  e5 
	CALL	_printf		; 0186 ...	  cd 01 04 
	POP	AF		; 0189 .	  f1 
	POP	AF		; 018a .	  f1 
	LD	HL,FFF8H	; 018b !..	  21 f8 ff 
	ADD	HL,DE		; 018e .	  19 
	LD	C,(HL)		; 018f N	  4e 
	INC	HL		; 0190 #	  23 
	LD	B,(HL)		; 0191 F	  46 
	PUSH	BC		; 0192 .	  c5 
	LD	HL,H0004	; 0193 !..	  21 04 00 
	PUSH	HL		; 0196 .	  e5 
	CALL	c.imod		; 0197 ...	  cd 00 04 
	POP	HL		; 019a .	  e1 
	LD	A,L		; 019b }	  7d 
	CP	03H		; 019c ..	  fe 03 
	JP	NZ,H01a4	; 019e ...	  c2 a4 01 
	LD	A,H		; 01a1 |	  7c 
	CP	00H		; 01a2 ..	  fe 00 
H01a4: 	JP	NZ,H0162	; 01a4 .b.	  c2 62 01 
	LD	HL,H0102	; 01a7 !..	  21 02 01 
	PUSH	HL		; 01aa .	  e5 
	CALL	_printf		; 01ab ...	  cd 01 04 
	POP	AF		; 01ae .	  f1 
	JP	H0162		; 01af .b.	  c3 62 01 

H01b2: 	DB	"   ",00H
H01b6: 	DB	" ",00H
H01b8: 	DB	" ",00H
H01ba: 	DB	"%02x ",00H
H01c0: 	DB	"%04x: ",00H
_hexdump: CALL	c.ent		; 01c7 ...	  cd 03 04 
	LD	HL,FFF7H	; 01ca !..	  21 f7 ff 
	ADD	HL,SP		; 01cd 9	  39 
	LD	SP,HL		; 01ce .	  f9 
	SUB	A		; 01cf .	  97 
	LD	(H0328),A	; 01d0 2(.	  32 28 03 
	LD	(H0329),A	; 01d3 2).	  32 29 03 
	LD	HL,FFF8H	; 01d6 !..	  21 f8 ff 
	ADD	HL,DE		; 01d9 .	  19 
	SUB	A		; 01da .	  97 
	LD	(HL),A		; 01db w	  77 
	INC	HL		; 01dc #	  23 
	LD	(HL),A		; 01dd w	  77 
H01de: 	LD	HL,H0006	; 01de !..	  21 06 00 
	ADD	HL,DE		; 01e1 .	  19 
	LD	A,(HL)		; 01e2 ~	  7e 
	INC	HL		; 01e3 #	  23 
	OR	(HL)		; 01e4 .	  b6 
	JP	Z,H0205		; 01e5 ...	  ca 05 02 
	LD	HL,H0328	; 01e8 !(.	  21 28 03 
	LD	A,(HL)		; 01eb ~	  7e 
	INC	HL		; 01ec #	  23 
	OR	(HL)		; 01ed .	  b6 
	JP	NZ,H0250	; 01ee .P.	  c2 50 02 
	LD	HL,H0004	; 01f1 !..	  21 04 00 
	ADD	HL,DE		; 01f4 .	  19 
	LD	C,(HL)		; 01f5 N	  4e 
	INC	HL		; 01f6 #	  23 
	LD	B,(HL)		; 01f7 F	  46 
	PUSH	BC		; 01f8 .	  c5 
	LD	HL,H01c0	; 01f9 !..	  21 c0 01 
	PUSH	HL		; 01fc .	  e5 
	CALL	_printf		; 01fd ...	  cd 01 04 
	POP	AF		; 0200 .	  f1 
	POP	AF		; 0201 .	  f1 
	JP	H0250		; 0202 .P.	  c3 50 02 

H0205: 	LD	HL,H0328	; 0205 !(.	  21 28 03 
	LD	A,(HL)		; 0208 ~	  7e 
	INC	HL		; 0209 #	  23 
	OR	(HL)		; 020a .	  b6 
	JP	Z,H02f7		; 020b ...	  ca f7 02 
	LD	HL,FFF8H	; 020e !..	  21 f8 ff 
	ADD	HL,DE		; 0211 .	  19 
	LD	A,(H0328)	; 0212 :(.	  3a 28 03 
	LD	(HL),A		; 0215 w	  77 
	LD	A,(H0329)	; 0216 :).	  3a 29 03 
	INC	HL		; 0219 #	  23 
	LD	(HL),A		; 021a w	  77 
H021b: 	LD	HL,FFF8H	; 021b !..	  21 f8 ff 
	ADD	HL,DE		; 021e .	  19 
	LD	A,(HL)		; 021f ~	  7e 
	SUB	10H		; 0220 ..	  d6 10 
	INC	HL		; 0222 #	  23 
	LD	A,(HL)		; 0223 ~	  7e 
	SBC	A,00H		; 0224 ..	  de 00 
	JP	P,H02fa		; 0226 ...	  f2 fa 02 
	LD	HL,FFF8H	; 0229 !..	  21 f8 ff 
	ADD	HL,DE		; 022c .	  19 
	LD	C,(HL)		; 022d N	  4e 
	INC	HL		; 022e #	  23 
	LD	B,(HL)		; 022f F	  46 
	PUSH	BC		; 0230 .	  c5 
	LD	HL,H0004	; 0231 !..	  21 04 00 
	PUSH	HL		; 0234 .	  e5 
	CALL	c.imod		; 0235 ...	  cd 00 04 
	POP	HL		; 0238 .	  e1 
	LD	A,L		; 0239 }	  7d 
	CP	03H		; 023a ..	  fe 03 
	JP	NZ,H0242	; 023c .B.	  c2 42 02 
	LD	A,H		; 023f |	  7c 
	CP	00H		; 0240 ..	  fe 00 
H0242: 	JP	NZ,H0300	; 0242 ...	  c2 00 03 
	LD	HL,H01b6	; 0245 !..	  21 b6 01 
	PUSH	HL		; 0248 .	  e5 
	CALL	_printf		; 0249 ...	  cd 01 04 
	POP	AF		; 024c .	  f1 
	JP	H0300		; 024d ...	  c3 00 03 

H0250: 	LD	HL,FFF7H	; 0250 !..	  21 f7 ff 
	ADD	HL,DE		; 0253 .	  19 
	PUSH	HL		; 0254 .	  e5 
	LD	HL,FFF8H	; 0255 !..	  21 f8 ff 
	ADD	HL,DE		; 0258 .	  19 
	LD	C,(HL)		; 0259 N	  4e 
	INC	HL		; 025a #	  23 
	LD	B,(HL)		; 025b F	  46 
	PUSH	BC		; 025c .	  c5 
	LD	HL,FFF8H	; 025d !..	  21 f8 ff 
	ADD	HL,DE		; 0260 .	  19 
	LD	A,(HL)		; 0261 ~	  7e 
	ADD	A,01H		; 0262 ..	  c6 01 
	LD	(HL),A		; 0264 w	  77 
	INC	HL		; 0265 #	  23 
	LD	A,(HL)		; 0266 ~	  7e 
	ADC	A,00H		; 0267 ..	  ce 00 
	LD	(HL),A		; 0269 w	  77 
	POP	HL		; 026a .	  e1 
	PUSH	HL		; 026b .	  e5 
	LD	HL,H0004	; 026c !..	  21 04 00 
	ADD	HL,DE		; 026f .	  19 
	LD	A,(HL)		; 0270 ~	  7e 
	INC	HL		; 0271 #	  23 
	LD	H,(HL)		; 0272 f	  66 
	LD	L,A		; 0273 o	  6f 
	EX	(SP),HL		; 0274 .	  e3 
	POP	BC		; 0275 .	  c1 
	ADD	HL,BC		; 0276 .	  09 
	POP	BC		; 0277 .	  c1 
	LD	A,(HL)		; 0278 ~	  7e 
	LD	(BC),A		; 0279 .	  02 
	LD	HL,H0318	; 027a !..	  21 18 03 
	PUSH	HL		; 027d .	  e5 
	LD	HL,(H0328)	; 027e *(.	  2a 28 03 
	EX	(SP),HL		; 0281 .	  e3 
	POP	BC		; 0282 .	  c1 
	ADD	HL,BC		; 0283 .	  09 
	PUSH	HL		; 0284 .	  e5 
	LD	HL,FFF7H	; 0285 !..	  21 f7 ff 
	ADD	HL,DE		; 0288 .	  19 
	POP	BC		; 0289 .	  c1 
	LD	A,(HL)		; 028a ~	  7e 
	LD	(BC),A		; 028b .	  02 
	LD	HL,FFF7H	; 028c !..	  21 f7 ff 
	ADD	HL,DE		; 028f .	  19 
	LD	A,(HL)		; 0290 ~	  7e 
	LD	C,A		; 0291 O	  4f 
	ADD	A,A		; 0292 .	  87 
	SBC	A,A		; 0293 .	  9f 
	LD	B,A		; 0294 G	  47 
	LD	A,C		; 0295 y	  79 
	AND	FFH		; 0296 ..	  e6 ff 
	LD	C,A		; 0298 O	  4f 
	SUB	A		; 0299 .	  97 
	LD	B,A		; 029a G	  47 
	PUSH	BC		; 029b .	  c5 
	LD	HL,H01ba	; 029c !..	  21 ba 01 
	PUSH	HL		; 029f .	  e5 
	CALL	_printf		; 02a0 ...	  cd 01 04 
	POP	AF		; 02a3 .	  f1 
	POP	AF		; 02a4 .	  f1 
	LD	HL,(H0328)	; 02a5 *(.	  2a 28 03 
	PUSH	HL		; 02a8 .	  e5 
	LD	HL,H0004	; 02a9 !..	  21 04 00 
	PUSH	HL		; 02ac .	  e5 
	CALL	c.imod		; 02ad ...	  cd 00 04 
	POP	HL		; 02b0 .	  e1 
	LD	A,L		; 02b1 }	  7d 
	CP	03H		; 02b2 ..	  fe 03 
	JP	NZ,H02ba	; 02b4 ...	  c2 ba 02 
	LD	A,H		; 02b7 |	  7c 
	CP	00H		; 02b8 ..	  fe 00 
H02ba: 	JP	NZ,H02c5	; 02ba ...	  c2 c5 02 
	LD	HL,H01b8	; 02bd !..	  21 b8 01 
	PUSH	HL		; 02c0 .	  e5 
	CALL	_printf		; 02c1 ...	  cd 01 04 
	POP	AF		; 02c4 .	  f1 
H02c5: 	LD	HL,H0006	; 02c5 !..	  21 06 00 
	ADD	HL,DE		; 02c8 .	  19 
	LD	A,(HL)		; 02c9 ~	  7e 
	SUB	01H		; 02ca ..	  d6 01 
	LD	(HL),A		; 02cc w	  77 
	INC	HL		; 02cd #	  23 
	LD	A,(HL)		; 02ce ~	  7e 
	SBC	A,00H		; 02cf ..	  de 00 
	LD	(HL),A		; 02d1 w	  77 
	LD	HL,(H0328)	; 02d2 *(.	  2a 28 03 
	PUSH	HL		; 02d5 .	  e5 
	LD	HL,(H0328)	; 02d6 *(.	  2a 28 03 
	INC	HL		; 02d9 #	  23 
	LD	(H0328),HL	; 02da "(.	  22 28 03 
	POP	HL		; 02dd .	  e1 
	LD	A,L		; 02de }	  7d 
	CP	0FH		; 02df ..	  fe 0f 
	JP	NZ,H02e7	; 02e1 ...	  c2 e7 02 
	LD	A,H		; 02e4 |	  7c 
	CP	00H		; 02e5 ..	  fe 00 
H02e7: 	JP	NZ,H01de	; 02e7 ...	  c2 de 01 
	CALL	_dumpasc	; 02ea ...	  cd 07 01 
	SUB	A		; 02ed .	  97 
	LD	(H0328),A	; 02ee 2(.	  32 28 03 
	LD	(H0329),A	; 02f1 2).	  32 29 03 
	JP	H01de		; 02f4 ...	  c3 de 01 

H02f7: 	JP	c.ret		; 02f7 ...	  c3 02 04 

H02fa: 	CALL	_dumpasc	; 02fa ...	  cd 07 01 
	JP	H02f7		; 02fd ...	  c3 f7 02 

H0300: 	LD	HL,H01b2	; 0300 !..	  21 b2 01 
	PUSH	HL		; 0303 .	  e5 
	CALL	_printf		; 0304 ...	  cd 01 04 
	POP	AF		; 0307 .	  f1 
	LD	HL,FFF8H	; 0308 !..	  21 f8 ff 
	ADD	HL,DE		; 030b .	  19 
	LD	A,(HL)		; 030c ~	  7e 
	ADD	A,01H		; 030d ..	  c6 01 
	LD	(HL),A		; 030f w	  77 
	INC	HL		; 0310 #	  23 
	LD	A,(HL)		; 0311 ~	  7e 
	ADC	A,00H		; 0312 ..	  ce 00 
	LD	(HL),A		; 0314 w	  77 
	JP	H021b		; 0315 ...	  c3 1b 02 


	org	0318H
H0318: 	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H
H0328: 	DW	H0000
