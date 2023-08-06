;
;	disas version 3
;	_signal.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 5 text: 0(98) data: 98(24) bss: 0
;
;	text	0000	0098
;	data	0098	0024
;	undef	0100	0002
;
c.ihl	equ	0100h
_errno	equ	0101h
;

	org	0000H
__signal: LD	HL,H0002	; 0000 !..	  21 02 00 
	ADD	HL,SP		; 0003 9	  39 
H0004: 	LD	A,(HL)		; 0004 ~	  7e 
	INC	HL		; 0005 #	  23 
	LD	H,(HL)		; 0006 f	  66 
	LD	L,A		; 0007 o	  6f 
	LD	(H009a),HL	; 0008 "..	  22 9a 00 
	LD	HL,H0004	; 000b !..	  21 04 00 
	ADD	HL,SP		; 000e 9	  39 
	LD	A,(HL)		; 000f ~	  7e 
	INC	HL		; 0010 #	  23 
	LD	H,(HL)		; 0011 f	  66 
	LD	L,A		; 0012 o	  6f 
	LD	(H009c),HL	; 0013 "..	  22 9c 00 
	SYS	indir 98 00 	; 0016 ....	  cf 00 98 00 
	LD	C,L		; 001a M	  4d 
	LD	B,H		; 001b D	  44 
	RET	NC		; 001c .	  d0 
	LD	BC,FFFFH	; 001d ...	  01 ff ff 
	LD	(__signal),HL	; 0020 "..	  22 00 00 
	RET			; 0023 .	  c9 

__jtab: PUSH	HL		; 0024 .	  e5 
	LD	HL,(H009e)	; 0025 *..	  2a 9e 00 
	JP	H008d		; 0028 ...	  c3 8d 00 

	DB	E5H,"*",A0H,00H,C3H,8DH,00H,E5H,"*",A2H,00H,C3H
	DB	8DH,00H,E5H,"*",A4H,00H,C3H,8DH,00H,E5H,"*",A6H
	DB	00H,C3H,8DH,00H,E5H,"*",A8H,00H,C3H,8DH,00H,E5H
	DB	"*",AAH,00H,C3H,8DH,00H,E5H,"*",ACH,00H,C3H,8DH
	DB	00H,E5H,"*",AEH,00H,C3H,8DH,00H,E5H,"*",B0H,00H
	DB	C3H,8DH,00H,E5H,"*",B2H,00H,C3H,8DH,00H,E5H,"*"
	DB	B4H,00H,C3H,8DH,00H,E5H,"*",B6H,00H,C3H,8DH,00H
	DB	E5H,"*",B8H,00H,C3H,8DH,00H,E5H,"*",BAH,00H,C3H
	DB	8DH,00H
H008d: 	PUSH	DE		; 008d .	  d5 
	PUSH	BC		; 008e .	  c5 
	PUSH	AF		; 008f .	  f5 
	CALL	__signal	; 0090 ...	  cd 00 00 
	POP	AF		; 0093 .	  f1 
	POP	BC		; 0094 .	  c1 
	POP	DE		; 0095 .	  d1 
	POP	HL		; 0096 .	  e1 
	RET			; 0097 .	  c9 


	org	0098H
	DB	CFH,"0"
H009a: 	DW	__signal
H009c: 	DW	__signal
H009e: 	DW	__signal
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H,00H
	DB	00H,00H,00H,00H
