;
;	disas version 3
;	perror.o
;
;	whitesmiths type 14 symlen 9 
;	symbols: 8 text: 0(333) data: 333(46) bss: 0
;
;	text	0000	0333
;	data	0333	0046
;	undef	0400	0007
;
c.r4	equ	0400h
c.ret	equ	0401h
c.ent	equ	0402h
c.rets	equ	0403h
c.ents	equ	0404h
_write	equ	0405h
_errno	equ	0406h
;

	org	0000H
H0000: 	NOP			; 0000 .	  00 
H0001: 	LD	B,D		; 0001 B	  42 
H0002: 	LD	(HL),D		; 0002 r	  72 
	LD	L,A		; 0003 o	  6f 
H0004: 	LD	L,E		; 0004 k	  6b 
	LD	H,L		; 0005 e	  65 
	LD	L,(HL)		; 0006 n	  6e 
	JR	NZ,H0079	; 0007 .p	  20 70 
	LD	L,C		; 0009 i	  69 
	LD	(HL),B		; 000a p	  70 
	LD	H,L		; 000b e	  65 
	NOP			; 000c .	  00 
	LD	D,H		; 000d T	  54 
	LD	L,A		; 000e o	  6f 
	LD	L,A		; 000f o	  6f 
	JR	NZ,H007f	; 0010 .m	  20 6d 
	LD	H,C		; 0012 a	  61 
	LD	L,(HL)		; 0013 n	  6e 
	LD	A,C		; 0014 y	  79 
	JR	NZ,H0083	; 0015 .l	  20 6c 
	LD	L,C		; 0017 i	  69 
	LD	L,(HL)		; 0018 n	  6e 
	LD	L,E		; 0019 k	  6b 
	LD	(HL),E		; 001a s	  73 
	NOP			; 001b .	  00 
	LD	D,D		; 001c R	  52 
	LD	H,L		; 001d e	  65 
	LD	H,C		; 001e a	  61 
	LD	H,H		; 001f d	  64 
H0020: 	DEC	L		; 0020 -	  2d 
	LD	L,A		; 0021 o	  6f 
	LD	L,(HL)		; 0022 n	  6e 
	LD	L,H		; 0023 l	  6c 
	LD	A,C		; 0024 y	  79 
	JR	NZ,H008d	; 0025 .f	  20 66 
	LD	L,C		; 0027 i	  69 
	LD	L,H		; 0028 l	  6c 
	LD	H,L		; 0029 e	  65 
	JR	NZ,H009f	; 002a .s	  20 73 
	LD	A,C		; 002c y	  79 
	LD	(HL),E		; 002d s	  73 
	LD	(HL),H		; 002e t	  74 
	LD	H,L		; 002f e	  65 
	LD	L,L		; 0030 m	  6d 
	NOP			; 0031 .	  00 
	LD	C,C		; 0032 I	  49 
	LD	L,H		; 0033 l	  6c 
	LD	L,H		; 0034 l	  6c 
	LD	H,L		; 0035 e	  65 
	LD	H,A		; 0036 g	  67 
	LD	H,C		; 0037 a	  61 
	LD	L,H		; 0038 l	  6c 
	JR	NZ,H00ae	; 0039 .s	  20 73 
	LD	H,L		; 003b e	  65 
	LD	H,L		; 003c e	  65 
	LD	L,E		; 003d k	  6b 
	NOP			; 003e .	  00 
	LD	C,(HL)		; 003f N	  4e 
	LD	L,A		; 0040 o	  6f 
	JR	NZ,H00b6	; 0041 .s	  20 73 
	LD	(HL),B		; 0043 p	  70 
	LD	H,C		; 0044 a	  61 
	LD	H,E		; 0045 c	  63 
	LD	H,L		; 0046 e	  65 
	JR	NZ,H00b5	; 0047 .l	  20 6c 
	LD	H,L		; 0049 e	  65 
	LD	H,(HL)		; 004a f	  66 
	LD	(HL),H		; 004b t	  74 
	JR	NZ,H00bd	; 004c .o	  20 6f 
	LD	L,(HL)		; 004e n	  6e 
	JR	NZ,H00b5	; 004f .d	  20 64 
	LD	H,L		; 0051 e	  65 
	HALT			; 0052 v	  76 
	LD	L,C		; 0053 i	  69 
	LD	H,E		; 0054 c	  63 
	LD	H,L		; 0055 e	  65 
	NOP			; 0056 .	  00 
	LD	B,(HL)		; 0057 F	  46 
	LD	L,C		; 0058 i	  69 
	LD	L,H		; 0059 l	  6c 
	LD	H,L		; 005a e	  65 
	JR	NZ,H00d1	; 005b .t	  20 74 
	LD	L,A		; 005d o	  6f 
	LD	L,A		; 005e o	  6f 
	JR	NZ,H00cd	; 005f .l	  20 6c 
	LD	H,C		; 0061 a	  61 
	LD	(HL),D		; 0062 r	  72 
	LD	H,A		; 0063 g	  67 
	LD	H,L		; 0064 e	  65 
	NOP			; 0065 .	  00 
	LD	D,H		; 0066 T	  54 
	LD	H,L		; 0067 e	  65 
	LD	A,B		; 0068 x	  78 
	LD	(HL),H		; 0069 t	  74 
	JR	NZ,H00d2	; 006a .f	  20 66 
	LD	L,C		; 006c i	  69 
	LD	L,H		; 006d l	  6c 
	LD	H,L		; 006e e	  65 
	JR	NZ,H00d3	; 006f .b	  20 62 
	LD	(HL),L		; 0071 u	  75 
	LD	(HL),E		; 0072 s	  73 
	LD	A,C		; 0073 y	  79 
	NOP			; 0074 .	  00 
	LD	C,(HL)		; 0075 N	  4e 
	LD	L,A		; 0076 o	  6f 
	LD	(HL),H		; 0077 t	  74 
	JR	NZ,H00db	; 0078 .a	  20 61 
	JR	NZ,H00f0	; 007a .t	  20 74 
	LD	A,C		; 007c y	  79 
	LD	(HL),B		; 007d p	  70 
	LD	H,L		; 007e e	  65 
H007f: 	LD	(HL),A		; 007f w	  77 
	LD	(HL),D		; 0080 r	  72 
	LD	L,C		; 0081 i	  69 
	LD	(HL),H		; 0082 t	  74 
H0083: 	LD	H,L		; 0083 e	  65 
	LD	(HL),D		; 0084 r	  72 
	NOP			; 0085 .	  00 
	LD	D,H		; 0086 T	  54 
	LD	L,A		; 0087 o	  6f 
	LD	L,A		; 0088 o	  6f 
	JR	NZ,H00f8	; 0089 .m	  20 6d 
	LD	H,C		; 008b a	  61 
	LD	L,(HL)		; 008c n	  6e 
H008d: 	LD	A,C		; 008d y	  79 
	JR	NZ,H00ff	; 008e .o	  20 6f 
	LD	(HL),B		; 0090 p	  70 
	LD	H,L		; 0091 e	  65 
	LD	L,(HL)		; 0092 n	  6e 
	JR	NZ,H00fb	; 0093 .f	  20 66 
	LD	L,C		; 0095 i	  69 
	LD	L,H		; 0096 l	  6c 
	LD	H,L		; 0097 e	  65 
	LD	(HL),E		; 0098 s	  73 
	NOP			; 0099 .	  00 
	LD	B,(HL)		; 009a F	  46 
	LD	L,C		; 009b i	  69 
	LD	L,H		; 009c l	  6c 
	LD	H,L		; 009d e	  65 
	JR	NZ,H0114	; 009e .t	  20 74 
	LD	H,C		; 00a0 a	  61 
	LD	H,D		; 00a1 b	  62 
	LD	L,H		; 00a2 l	  6c 
	LD	H,L		; 00a3 e	  65 
	JR	NZ,H0115	; 00a4 .o	  20 6f 
	HALT			; 00a6 v	  76 
	LD	H,L		; 00a7 e	  65 
	LD	(HL),D		; 00a8 r	  72 
	LD	H,(HL)		; 00a9 f	  66 
	LD	L,H		; 00aa l	  6c 
	LD	L,A		; 00ab o	  6f 
	LD	(HL),A		; 00ac w	  77 
	NOP			; 00ad .	  00 
H00ae: 	LD	C,C		; 00ae I	  49 
	LD	L,(HL)		; 00af n	  6e 
	HALT			; 00b0 v	  76 
	LD	H,C		; 00b1 a	  61 
	LD	L,H		; 00b2 l	  6c 
	LD	L,C		; 00b3 i	  69 
	LD	H,H		; 00b4 d	  64 
H00b5: 	JR	NZ,H0118	; 00b5 .a	  20 61 
	LD	(HL),D		; 00b7 r	  72 
	LD	H,A		; 00b8 g	  67 
	LD	(HL),L		; 00b9 u	  75 
	LD	L,L		; 00ba m	  6d 
	LD	H,L		; 00bb e	  65 
	LD	L,(HL)		; 00bc n	  6e 
H00bd: 	LD	(HL),H		; 00bd t	  74 
	NOP			; 00be .	  00 
	LD	C,C		; 00bf I	  49 
	LD	(HL),E		; 00c0 s	  73 
	JR	NZ,H0124	; 00c1 .a	  20 61 
	JR	NZ,H0129	; 00c3 .d	  20 64 
	LD	L,C		; 00c5 i	  69 
	LD	(HL),D		; 00c6 r	  72 
	LD	H,L		; 00c7 e	  65 
	LD	H,E		; 00c8 c	  63 
	LD	(HL),H		; 00c9 t	  74 
	LD	L,A		; 00ca o	  6f 
	LD	(HL),D		; 00cb r	  72 
	LD	A,C		; 00cc y	  79 
H00cd: 	NOP			; 00cd .	  00 
	LD	C,(HL)		; 00ce N	  4e 
	LD	L,A		; 00cf o	  6f 
	LD	(HL),H		; 00d0 t	  74 
H00d1: 	JR	NZ,H0134	; 00d1 .a	  20 61 
H00d3: 	JR	NZ,H0139	; 00d3 .d	  20 64 
	LD	L,C		; 00d5 i	  69 
	LD	(HL),D		; 00d6 r	  72 
	LD	H,L		; 00d7 e	  65 
	LD	H,E		; 00d8 c	  63 
	LD	(HL),H		; 00d9 t	  74 
	LD	L,A		; 00da o	  6f 
H00db: 	LD	(HL),D		; 00db r	  72 
	LD	A,C		; 00dc y	  79 
	NOP			; 00dd .	  00 
	LD	C,(HL)		; 00de N	  4e 
	LD	L,A		; 00df o	  6f 
	JR	NZ,H0155	; 00e0 .s	  20 73 
	LD	(HL),L		; 00e2 u	  75 
	LD	H,E		; 00e3 c	  63 
	LD	L,B		; 00e4 h	  68 
	JR	NZ,H014b	; 00e5 .d	  20 64 
	LD	H,L		; 00e7 e	  65 
	HALT			; 00e8 v	  76 
	LD	L,C		; 00e9 i	  69 
	LD	H,E		; 00ea c	  63 
	LD	H,L		; 00eb e	  65 
	NOP			; 00ec .	  00 
	LD	B,E		; 00ed C	  43 
	LD	(HL),D		; 00ee r	  72 
	LD	L,A		; 00ef o	  6f 
H00f0: 	LD	(HL),E		; 00f0 s	  73 
	LD	(HL),E		; 00f1 s	  73 
	DEC	L		; 00f2 -	  2d 
	LD	H,H		; 00f3 d	  64 
	LD	H,L		; 00f4 e	  65 
	HALT			; 00f5 v	  76 
	LD	L,C		; 00f6 i	  69 
	LD	H,E		; 00f7 c	  63 
H00f8: 	LD	H,L		; 00f8 e	  65 
	JR	NZ,H0167	; 00f9 .l	  20 6c 
H00fb: 	LD	L,C		; 00fb i	  69 
	LD	L,(HL)		; 00fc n	  6e 
	LD	L,E		; 00fd k	  6b 
	NOP			; 00fe .	  00 
H00ff: 	LD	B,(HL)		; 00ff F	  46 
	LD	L,C		; 0100 i	  69 
	LD	L,H		; 0101 l	  6c 
	LD	H,L		; 0102 e	  65 
	JR	NZ,H016a	; 0103 .e	  20 65 
	LD	A,B		; 0105 x	  78 
	LD	L,C		; 0106 i	  69 
	LD	(HL),E		; 0107 s	  73 
	LD	(HL),H		; 0108 t	  74 
	LD	(HL),E		; 0109 s	  73 
	NOP			; 010a .	  00 
	LD	B,(HL)		; 010b F	  46 
	LD	L,C		; 010c i	  69 
	LD	L,H		; 010d l	  6c 
	LD	H,L		; 010e e	  65 
	JR	NZ,H0180	; 010f .o	  20 6f 
	LD	(HL),D		; 0111 r	  72 
	JR	NZ,H0178	; 0112 .d	  20 64 
H0114: 	LD	H,L		; 0114 e	  65 
H0115: 	HALT			; 0115 v	  76 
	LD	L,C		; 0116 i	  69 
	LD	H,E		; 0117 c	  63 
H0118: 	LD	H,L		; 0118 e	  65 
	JR	NZ,H017d	; 0119 .b	  20 62 
	LD	(HL),L		; 011b u	  75 
	LD	(HL),E		; 011c s	  73 
	LD	A,C		; 011d y	  79 
	NOP			; 011e .	  00 
	LD	B,D		; 011f B	  42 
	LD	L,H		; 0120 l	  6c 
	LD	L,A		; 0121 o	  6f 
	LD	H,E		; 0122 c	  63 
	LD	L,E		; 0123 k	  6b 
H0124: 	JR	NZ,H018a	; 0124 .d	  20 64 
	LD	H,L		; 0126 e	  65 
	HALT			; 0127 v	  76 
	LD	L,C		; 0128 i	  69 
H0129: 	LD	H,E		; 0129 c	  63 
	LD	H,L		; 012a e	  65 
	JR	NZ,H019f	; 012b .r	  20 72 
	LD	H,L		; 012d e	  65 
	LD	(HL),C		; 012e q	  71 
	LD	(HL),L		; 012f u	  75 
	LD	L,C		; 0130 i	  69 
	LD	(HL),D		; 0131 r	  72 
	LD	H,L		; 0132 e	  65 
	LD	H,H		; 0133 d	  64 
H0134: 	NOP			; 0134 .	  00 
	NOP			; 0135 .	  00 
	LD	D,B		; 0136 P	  50 
	LD	H,L		; 0137 e	  65 
	LD	(HL),D		; 0138 r	  72 
H0139: 	LD	L,L		; 0139 m	  6d 
	LD	L,C		; 013a i	  69 
	LD	(HL),E		; 013b s	  73 
	LD	(HL),E		; 013c s	  73 
	LD	L,C		; 013d i	  69 
	LD	L,A		; 013e o	  6f 
	LD	L,(HL)		; 013f n	  6e 
	JR	NZ,H01a6	; 0140 .d	  20 64 
	LD	H,L		; 0142 e	  65 
	LD	L,(HL)		; 0143 n	  6e 
	LD	L,C		; 0144 i	  69 
	LD	H,L		; 0145 e	  65 
	LD	H,H		; 0146 d	  64 
	NOP			; 0147 .	  00 
	LD	C,(HL)		; 0148 N	  4e 
	LD	L,A		; 0149 o	  6f 
	LD	(HL),H		; 014a t	  74 
H014b: 	JR	NZ,H01b2	; 014b .e	  20 65 
	LD	L,(HL)		; 014d n	  6e 
	LD	L,A		; 014e o	  6f 
	LD	(HL),L		; 014f u	  75 
	LD	H,A		; 0150 g	  67 
	LD	L,B		; 0151 h	  68 
	JR	NZ,H01c1	; 0152 .m	  20 6d 
	LD	H,L		; 0154 e	  65 
H0155: 	LD	L,L		; 0155 m	  6d 
	LD	L,A		; 0156 o	  6f 
	LD	(HL),D		; 0157 r	  72 
	LD	A,C		; 0158 y	  79 
	NOP			; 0159 .	  00 
	LD	C,(HL)		; 015a N	  4e 
	LD	L,A		; 015b o	  6f 
	JR	NZ,H01cb	; 015c .m	  20 6d 
	LD	L,A		; 015e o	  6f 
	LD	(HL),D		; 015f r	  72 
	LD	H,L		; 0160 e	  65 
	JR	NZ,H01d3	; 0161 .p	  20 70 
	LD	(HL),D		; 0163 r	  72 
	LD	L,A		; 0164 o	  6f 
	LD	H,E		; 0165 c	  63 
	LD	H,L		; 0166 e	  65 
H0167: 	LD	(HL),E		; 0167 s	  73 
	LD	(HL),E		; 0168 s	  73 
	LD	H,L		; 0169 e	  65 
H016a: 	LD	(HL),E		; 016a s	  73 
	NOP			; 016b .	  00 
	LD	C,(HL)		; 016c N	  4e 
	LD	L,A		; 016d o	  6f 
	JR	NZ,H01d3	; 016e .c	  20 63 
	LD	L,B		; 0170 h	  68 
	LD	L,C		; 0171 i	  69 
	LD	L,H		; 0172 l	  6c 
	LD	H,H		; 0173 d	  64 
	LD	(HL),D		; 0174 r	  72 
	LD	H,L		; 0175 e	  65 
	LD	L,(HL)		; 0176 n	  6e 
	NOP			; 0177 .	  00 
H0178: 	LD	B,D		; 0178 B	  42 
	LD	H,C		; 0179 a	  61 
	LD	H,H		; 017a d	  64 
	JR	NZ,H01e3	; 017b .f	  20 66 
H017d: 	LD	L,C		; 017d i	  69 
	LD	L,H		; 017e l	  6c 
	LD	H,L		; 017f e	  65 
H0180: 	JR	NZ,H01f0	; 0180 .n	  20 6e 
	LD	(HL),L		; 0182 u	  75 
	LD	L,L		; 0183 m	  6d 
	LD	H,D		; 0184 b	  62 
	LD	H,L		; 0185 e	  65 
	LD	(HL),D		; 0186 r	  72 
	NOP			; 0187 .	  00 
	LD	B,L		; 0188 E	  45 
	LD	A,B		; 0189 x	  78 
H018a: 	LD	H,L		; 018a e	  65 
	LD	H,E		; 018b c	  63 
	JR	NZ,H01f4	; 018c .f	  20 66 
	LD	L,A		; 018e o	  6f 
	LD	(HL),D		; 018f r	  72 
	LD	L,L		; 0190 m	  6d 
	LD	H,C		; 0191 a	  61 
	LD	(HL),H		; 0192 t	  74 
	JR	NZ,H01fa	; 0193 .e	  20 65 
	LD	(HL),D		; 0195 r	  72 
	LD	(HL),D		; 0196 r	  72 
	LD	L,A		; 0197 o	  6f 
	LD	(HL),D		; 0198 r	  72 
	NOP			; 0199 .	  00 
	LD	B,C		; 019a A	  41 
	LD	(HL),D		; 019b r	  72 
	LD	H,A		; 019c g	  67 
	JR	NZ,H020b	; 019d .l	  20 6c 
H019f: 	LD	L,C		; 019f i	  69 
	LD	(HL),E		; 01a0 s	  73 
	LD	(HL),H		; 01a1 t	  74 
	JR	NZ,H0218	; 01a2 .t	  20 74 
	LD	L,A		; 01a4 o	  6f 
	LD	L,A		; 01a5 o	  6f 
H01a6: 	JR	NZ,H0214	; 01a6 .l	  20 6c 
	LD	L,A		; 01a8 o	  6f 
	LD	L,(HL)		; 01a9 n	  6e 
	LD	H,A		; 01aa g	  67 
	NOP			; 01ab .	  00 
	LD	C,(HL)		; 01ac N	  4e 
	LD	L,A		; 01ad o	  6f 
	JR	NZ,H0223	; 01ae .s	  20 73 
	LD	(HL),L		; 01b0 u	  75 
	LD	H,E		; 01b1 c	  63 
H01b2: 	LD	L,B		; 01b2 h	  68 
	JR	NZ,H0219	; 01b3 .d	  20 64 
	LD	H,L		; 01b5 e	  65 
	HALT			; 01b6 v	  76 
	LD	L,C		; 01b7 i	  69 
	LD	H,E		; 01b8 c	  63 
	LD	H,L		; 01b9 e	  65 
	JR	NZ,H022b	; 01ba .o	  20 6f 
	LD	(HL),D		; 01bc r	  72 
	JR	NZ,H0220	; 01bd .a	  20 61 
	LD	H,H		; 01bf d	  64 
	LD	H,H		; 01c0 d	  64 
H01c1: 	LD	(HL),D		; 01c1 r	  72 
	LD	H,L		; 01c2 e	  65 
	LD	(HL),E		; 01c3 s	  73 
	LD	(HL),E		; 01c4 s	  73 
	NOP			; 01c5 .	  00 
	LD	C,C		; 01c6 I	  49 
	CPL			; 01c7 /	  2f 
	LD	C,A		; 01c8 O	  4f 
	JR	NZ,H0230	; 01c9 .e	  20 65 
H01cb: 	LD	(HL),D		; 01cb r	  72 
	LD	(HL),D		; 01cc r	  72 
	LD	L,A		; 01cd o	  6f 
	LD	(HL),D		; 01ce r	  72 
	NOP			; 01cf .	  00 
	LD	C,C		; 01d0 I	  49 
	LD	L,(HL)		; 01d1 n	  6e 
	LD	(HL),H		; 01d2 t	  74 
H01d3: 	LD	H,L		; 01d3 e	  65 
	LD	(HL),D		; 01d4 r	  72 
	LD	(HL),D		; 01d5 r	  72 
	LD	(HL),L		; 01d6 u	  75 
	LD	(HL),B		; 01d7 p	  70 
	LD	(HL),H		; 01d8 t	  74 
	LD	H,L		; 01d9 e	  65 
	LD	H,H		; 01da d	  64 
	JR	NZ,H0250	; 01db .s	  20 73 
	LD	A,C		; 01dd y	  79 
	LD	(HL),E		; 01de s	  73 
	LD	(HL),H		; 01df t	  74 
	LD	H,L		; 01e0 e	  65 
	LD	L,L		; 01e1 m	  6d 
	JR	NZ,H0247	; 01e2 .c	  20 63 
	LD	H,C		; 01e4 a	  61 
	LD	L,H		; 01e5 l	  6c 
	LD	L,H		; 01e6 l	  6c 
	NOP			; 01e7 .	  00 
	LD	C,(HL)		; 01e8 N	  4e 
	LD	L,A		; 01e9 o	  6f 
	JR	NZ,H025f	; 01ea .s	  20 73 
	LD	(HL),L		; 01ec u	  75 
	LD	H,E		; 01ed c	  63 
	LD	L,B		; 01ee h	  68 
	JR	NZ,H0261	; 01ef .p	  20 70 
	LD	(HL),D		; 01f1 r	  72 
	LD	L,A		; 01f2 o	  6f 
	LD	H,E		; 01f3 c	  63 
H01f4: 	LD	H,L		; 01f4 e	  65 
	LD	(HL),E		; 01f5 s	  73 
	LD	(HL),E		; 01f6 s	  73 
	NOP			; 01f7 .	  00 
	LD	C,(HL)		; 01f8 N	  4e 
	LD	L,A		; 01f9 o	  6f 
H01fa: 	JR	NZ,H026f	; 01fa .s	  20 73 
	LD	(HL),L		; 01fc u	  75 
	LD	H,E		; 01fd c	  63 
	LD	L,B		; 01fe h	  68 
	JR	NZ,H0267	; 01ff .f	  20 66 
	LD	L,C		; 0201 i	  69 
	LD	L,H		; 0202 l	  6c 
	LD	H,L		; 0203 e	  65 
	JR	NZ,H0275	; 0204 .o	  20 6f 
	LD	(HL),D		; 0206 r	  72 
	JR	NZ,H026d	; 0207 .d	  20 64 
	LD	L,C		; 0209 i	  69 
	LD	(HL),D		; 020a r	  72 
H020b: 	LD	H,L		; 020b e	  65 
	LD	H,E		; 020c c	  63 
	LD	(HL),H		; 020d t	  74 
	LD	L,A		; 020e o	  6f 
	LD	(HL),D		; 020f r	  72 
	LD	A,C		; 0210 y	  79 
	NOP			; 0211 .	  00 
	LD	C,(HL)		; 0212 N	  4e 
	LD	L,A		; 0213 o	  6f 
H0214: 	LD	(HL),H		; 0214 t	  74 
	JR	NZ,H028a	; 0215 .s	  20 73 
	LD	(HL),L		; 0217 u	  75 
H0218: 	LD	(HL),B		; 0218 p	  70 
H0219: 	LD	H,L		; 0219 e	  65 
	LD	(HL),D		; 021a r	  72 
	DEC	L		; 021b -	  2d 
	LD	(HL),L		; 021c u	  75 
	LD	(HL),E		; 021d s	  73 
	LD	H,L		; 021e e	  65 
	LD	(HL),D		; 021f r	  72 
H0220: 	NOP			; 0220 .	  00 
	LD	D,L		; 0221 U	  55 
	LD	L,(HL)		; 0222 n	  6e 
H0223: 	LD	L,E		; 0223 k	  6b 
	LD	L,(HL)		; 0224 n	  6e 
	LD	L,A		; 0225 o	  6f 
	LD	(HL),A		; 0226 w	  77 
	LD	L,(HL)		; 0227 n	  6e 
	JR	NZ,H028f	; 0228 .e	  20 65 
	LD	(HL),D		; 022a r	  72 
H022b: 	LD	(HL),D		; 022b r	  72 
	LD	L,A		; 022c o	  6f 
	LD	(HL),D		; 022d r	  72 
	NOP			; 022e .	  00 
H022f: 	LD	A,(BC)		; 022f .	  0a 
H0230: 	NOP			; 0230 .	  00 
H0231: 	LD	A,(H0020)	; 0231 :..	  3a 20 00 
_perror: CALL	H0000		; 0234 ...	  cd 00 00 
	PUSH	AF		; 0237 .	  f5 
	PUSH	AF		; 0238 .	  f5 
	PUSH	AF		; 0239 .	  f5 
	PUSH	AF		; 023a .	  f5 
	LD	HL,H0004	; 023b !..	  21 04 00 
	ADD	HL,DE		; 023e .	  19 
	LD	A,(HL)		; 023f ~	  7e 
	INC	HL		; 0240 #	  23 
	OR	(HL)		; 0241 .	  b6 
	JP	Z,H0283		; 0242 ...	  ca 83 02 
	LD	HL,H0004	; 0245 !..	  21 04 00 
	ADD	HL,DE		; 0248 .	  19 
	LD	A,(HL)		; 0249 ~	  7e 
	INC	HL		; 024a #	  23 
	LD	H,(HL)		; 024b f	  66 
	LD	L,A		; 024c o	  6f 
	LD	A,(HL)		; 024d ~	  7e 
	OR	A		; 024e .	  b7 
	JP	Z,H0283		; 024f ...	  ca 83 02 
	LD	HL,H0004	; 0252 !..	  21 04 00 
	ADD	HL,DE		; 0255 .	  19 
	LD	C,(HL)		; 0256 N	  4e 
	INC	HL		; 0257 #	  23 
	LD	B,(HL)		; 0258 F	  46 
	PUSH	BC		; 0259 .	  c5 
	CALL	H02fb		; 025a ...	  cd fb 02 
	POP	AF		; 025d .	  f1 
	PUSH	BC		; 025e .	  c5 
H025f: 	LD	HL,H0004	; 025f !..	  21 04 00 
	ADD	HL,DE		; 0262 .	  19 
	LD	C,(HL)		; 0263 N	  4e 
	INC	HL		; 0264 #	  23 
	LD	B,(HL)		; 0265 F	  46 
	PUSH	BC		; 0266 .	  c5 
H0267: 	LD	HL,H0002	; 0267 !..	  21 02 00 
	PUSH	HL		; 026a .	  e5 
	CALL	H0000		; 026b ...	  cd 00 00 
	POP	AF		; 026e .	  f1 
H026f: 	POP	AF		; 026f .	  f1 
	POP	AF		; 0270 .	  f1 
	LD	HL,H0002	; 0271 !..	  21 02 00 
	PUSH	HL		; 0274 .	  e5 
H0275: 	LD	HL,H0231	; 0275 !1.	  21 31 02 
	PUSH	HL		; 0278 .	  e5 
	LD	HL,H0002	; 0279 !..	  21 02 00 
	PUSH	HL		; 027c .	  e5 
	CALL	H0000		; 027d ...	  cd 00 00 
	POP	AF		; 0280 .	  f1 
	POP	AF		; 0281 .	  f1 
	POP	AF		; 0282 .	  f1 
H0283: 	LD	A,(H0000)	; 0283 :..	  3a 00 00 
	SUB	01H		; 0286 ..	  d6 01 
	LD	A,(H0001)	; 0288 :..	  3a 01 00 
	SBC	A,00H		; 028b ..	  de 00 
	JP	M,H02ad		; 028d ...	  fa ad 02 
	LD	HL,H0000	; 0290 !..	  21 00 00 
	LD	A,20H		; 0293 >.	  3e 20 
	SUB	(HL)		; 0295 .	  96 
	LD	A,00H		; 0296 >.	  3e 00 
	INC	HL		; 0298 #	  23 
	SBC	A,(HL)		; 0299 .	  9e 
	JP	M,H02ad		; 029a ...	  fa ad 02 
	LD	HL,FFF8H	; 029d !..	  21 f8 ff 
	ADD	HL,DE		; 02a0 .	  19 
	LD	A,(H0000)	; 02a1 :..	  3a 00 00 
	LD	(HL),A		; 02a4 w	  77 
	LD	A,(H0001)	; 02a5 :..	  3a 01 00 
	INC	HL		; 02a8 #	  23 
	LD	(HL),A		; 02a9 w	  77 
	JP	H02b5		; 02aa ...	  c3 b5 02 

H02ad: 	LD	HL,FFF8H	; 02ad !..	  21 f8 ff 
	ADD	HL,DE		; 02b0 .	  19 
	SUB	A		; 02b1 .	  97 
	LD	(HL),A		; 02b2 w	  77 
	INC	HL		; 02b3 #	  23 
	LD	(HL),A		; 02b4 w	  77 
H02b5: 	LD	HL,FFF8H	; 02b5 !..	  21 f8 ff 
	ADD	HL,DE		; 02b8 .	  19 
	LD	A,(HL)		; 02b9 ~	  7e 
	INC	HL		; 02ba #	  23 
	LD	H,(HL)		; 02bb f	  66 
	LD	L,A		; 02bc o	  6f 
	ADD	HL,HL		; 02bd )	  29 
	LD	BC,H0333	; 02be .3.	  01 33 03 
	ADD	HL,BC		; 02c1 .	  09 
	LD	C,(HL)		; 02c2 N	  4e 
	INC	HL		; 02c3 #	  23 
	LD	B,(HL)		; 02c4 F	  46 
	PUSH	BC		; 02c5 .	  c5 
	CALL	H02fb		; 02c6 ...	  cd fb 02 
	POP	AF		; 02c9 .	  f1 
	PUSH	BC		; 02ca .	  c5 
	LD	HL,FFF8H	; 02cb !..	  21 f8 ff 
	ADD	HL,DE		; 02ce .	  19 
	LD	A,(HL)		; 02cf ~	  7e 
	INC	HL		; 02d0 #	  23 
	LD	H,(HL)		; 02d1 f	  66 
	LD	L,A		; 02d2 o	  6f 
	ADD	HL,HL		; 02d3 )	  29 
	LD	BC,H0333	; 02d4 .3.	  01 33 03 
	ADD	HL,BC		; 02d7 .	  09 
	LD	C,(HL)		; 02d8 N	  4e 
	INC	HL		; 02d9 #	  23 
	LD	B,(HL)		; 02da F	  46 
	PUSH	BC		; 02db .	  c5 
	LD	HL,H0002	; 02dc !..	  21 02 00 
	PUSH	HL		; 02df .	  e5 
	CALL	H0000		; 02e0 ...	  cd 00 00 
	POP	AF		; 02e3 .	  f1 
	POP	AF		; 02e4 .	  f1 
	POP	AF		; 02e5 .	  f1 
	LD	HL,H0001	; 02e6 !..	  21 01 00 
	PUSH	HL		; 02e9 .	  e5 
	LD	HL,H022f	; 02ea !/.	  21 2f 02 
	PUSH	HL		; 02ed .	  e5 
	LD	HL,H0002	; 02ee !..	  21 02 00 
	PUSH	HL		; 02f1 .	  e5 
	CALL	H0000		; 02f2 ...	  cd 00 00 
	POP	AF		; 02f5 .	  f1 
	POP	AF		; 02f6 .	  f1 
	POP	AF		; 02f7 .	  f1 
	JP	H0000		; 02f8 ...	  c3 00 00 

H02fb: 	CALL	H0000		; 02fb ...	  cd 00 00 
	LD	HL,H0004	; 02fe !..	  21 04 00 
	ADD	HL,DE		; 0301 .	  19 
	LD	A,(HL)		; 0302 ~	  7e 
	INC	HL		; 0303 #	  23 
	LD	H,(HL)		; 0304 f	  66 
	LD	L,A		; 0305 o	  6f 
	LD	(H0000),HL	; 0306 "..	  22 00 00 
	LD	HL,(H0000)	; 0309 *..	  2a 00 00 
	LD	(H0377),HL	; 030c "w.	  22 77 03 
H030f: 	LD	HL,(H0377)	; 030f *w.	  2a 77 03 
	LD	A,(HL)		; 0312 ~	  7e 
	OR	A		; 0313 .	  b7 
	JP	Z,H0321		; 0314 .!.	  ca 21 03 
	LD	HL,(H0377)	; 0317 *w.	  2a 77 03 
	INC	HL		; 031a #	  23 
	LD	(H0377),HL	; 031b "w.	  22 77 03 
	JP	H030f		; 031e ...	  c3 0f 03 

H0321: 	LD	HL,(H0377)	; 0321 *w.	  2a 77 03 
	PUSH	HL		; 0324 .	  e5 
	LD	HL,H0000	; 0325 !..	  21 00 00 
	POP	BC		; 0328 .	  c1 
	LD	A,C		; 0329 y	  79 
	SUB	(HL)		; 032a .	  96 
	LD	C,A		; 032b O	  4f 
	LD	A,B		; 032c x	  78 
	INC	HL		; 032d #	  23 
	SBC	A,(HL)		; 032e .	  9e 
	LD	B,A		; 032f G	  47 
	JP	H0000		; 0330 ...	  c3 00 00 


	org	0333H
H0333: 	DB	"!",02H,12H,02H,F8H,01H,E8H,01H,D0H,01H,C6H,01H
	DB	ACH,01H,9AH,01H,88H,01H,"x",01H,"l",01H,"Z",01H
	DB	"H",01H,"6",01H,"5",01H,1FH,01H,0BH,01H,FFH,00H
	DB	EDH,00H,DEH,00H,CEH,00H,BFH,00H,AEH,00H,9AH,00H
	DB	86H,00H,"u",00H,"f",00H,"W",00H,"?",00H,"2",00H
	DB	1CH,00H,0DH,00H,01H,00H,00H,00H
H0377: 	DW	H0000
