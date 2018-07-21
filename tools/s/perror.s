# z80 disassembly
L0_0:
	NOP                            ; 0000: 00                 .     
L0_1:
	LD B,D                         ; 0001: 42                 B     
	LD (HL),D                      ; 0002: 72                 r     
	LD L,A                         ;       6f                 o     
	LD L,E                         ; 0004: 6b                 k     
	LD H,L                         ;       65                 e     
	LD L,(HL)                      ;       6e                 n     
	JR NZ,0x0079                   ;       20 70              .p    
	LD L,C                         ;       69                 i     
	LD (HL),B                      ;       70                 p     
	LD H,L                         ;       65                 e     
L0_c:
	NOP                            ;       00                 .     
L0_d:
	LD D,H                         ;       54                 T     
	LD L,A                         ;       6f                 o     
	LD L,A                         ;       6f                 o     
	JR NZ,0x007f                   ;       20 6d              .m    
	LD H,C                         ;       61                 a     
	LD L,(HL)                      ;       6e                 n     
L0_14:
	LD A,C                         ;       79                 y     
	JR NZ,0x0083                   ;       20 6c              .l    
	LD L,C                         ;       69                 i     
	LD L,(HL)                      ;       6e                 n     
	LD L,E                         ;       6b                 k     
	LD (HL),E                      ;       73                 s     
	NOP                            ;       00                 .     
L0_1c:
	LD D,D                         ;       52                 R     
	LD H,L                         ;       65                 e     
	LD H,C                         ;       61                 a     
	LD H,H                         ;       64                 d     
L0_20:
	DEC L                          ; 0020: 2d                 -     
L0_21:
	LD L,A                         ;       6f                 o     
	LD L,(HL)                      ;       6e                 n     
	LD L,H                         ;       6c                 l     
	LD A,C                         ;       79                 y     
L0_25:
	JR NZ,0x008d                   ;       20 66              .f    
	LD L,C                         ;       69                 i     
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
	JR NZ,0x009f                   ;       20 73              .s    
	LD A,C                         ;       79                 y     
	LD (HL),E                      ;       73                 s     
	LD (HL),H                      ;       74                 t     
	LD H,L                         ;       65                 e     
	LD L,L                         ;       6d                 m     
	NOP                            ;       00                 .     
L0_32:
	LD C,C                         ;       49                 I     
	LD L,H                         ;       6c                 l     
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
L0_36:
	LD H,A                         ;       67                 g     
L0_37:
	LD H,C                         ;       61                 a     
	LD L,H                         ;       6c                 l     
	JR NZ,0x00ae                   ;       20 73              .s    
	LD H,L                         ;       65                 e     
	LD H,L                         ;       65                 e     
	LD L,E                         ;       6b                 k     
	NOP                            ;       00                 .     
L0_3f:
	LD C,(HL)                      ;       4e                 N     
	LD L,A                         ;       6f                 o     
	JR NZ,0x00b6                   ;       20 73              .s    
	LD (HL),B                      ;       70                 p     
	LD H,C                         ;       61                 a     
	LD H,E                         ;       63                 c     
	LD H,L                         ;       65                 e     
	JR NZ,0x00b5                   ;       20 6c              .l    
L0_49:
	LD H,L                         ;       65                 e     
	LD H,(HL)                      ;       66                 f     
	LD (HL),H                      ;       74                 t     
	JR NZ,0x00bd                   ;       20 6f              .o    
	LD L,(HL)                      ;       6e                 n     
	JR NZ,0x00b5                   ;       20 64              .d    
	LD H,L                         ;       65                 e     
	HALT                           ;       76                 v     
	LD L,C                         ;       69                 i     
	LD H,E                         ;       63                 c     
	LD H,L                         ;       65                 e     
	NOP                            ;       00                 .     
L0_57:
	LD B,(HL)                      ;       46                 F     
	LD L,C                         ;       69                 i     
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
L0_5b:
	JR NZ,0x00d1                   ;       20 74              .t    
	LD L,A                         ;       6f                 o     
	LD L,A                         ;       6f                 o     
	JR NZ,0x00cd                   ;       20 6c              .l    
	LD H,C                         ;       61                 a     
	LD (HL),D                      ;       72                 r     
	LD H,A                         ;       67                 g     
	LD H,L                         ;       65                 e     
	NOP                            ;       00                 .     
L0_66:
	LD D,H                         ;       54                 T     
	LD H,L                         ;       65                 e     
	LD A,B                         ;       78                 x     
	LD (HL),H                      ;       74                 t     
	JR NZ,0x00d2                   ;       20 66              .f    
	LD L,C                         ;       69                 i     
L0_6d:
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
	JR NZ,0x00d3                   ;       20 62              .b    
	LD (HL),L                      ;       75                 u     
	LD (HL),E                      ;       73                 s     
	LD A,C                         ;       79                 y     
	NOP                            ;       00                 .     
L0_75:
	LD C,(HL)                      ;       4e                 N     
	LD L,A                         ;       6f                 o     
	LD (HL),H                      ;       74                 t     
	JR NZ,L0_79                    ;       20 61              .a    
	JR NZ,0x00f0                   ;       20 74              .t    
	LD A,C                         ;       79                 y     
	LD (HL),B                      ;       70                 p     
	LD H,L                         ;       65                 e     
	LD (HL),A                      ; 007f: 77                 w     
	LD (HL),D                      ;       72                 r     
	LD L,C                         ;       69                 i     
	LD (HL),H                      ;       74                 t     
	LD H,L                         ; 0083: 65                 e     
	LD (HL),D                      ;       72                 r     
	NOP                            ;       00                 .     
L0_86:
	LD D,H                         ;       54                 T     
	LD L,A                         ;       6f                 o     
	LD L,A                         ;       6f                 o     
L0_89:
	JR NZ,0x00f8                   ;       20 6d              .m    
	LD H,C                         ;       61                 a     
	LD L,(HL)                      ;       6e                 n     
	LD A,C                         ; 008d: 79                 y     
	JR NZ,0x00ff                   ;       20 6f              .o    
	LD (HL),B                      ;       70                 p     
	LD H,L                         ;       65                 e     
	LD L,(HL)                      ;       6e                 n     
	JR NZ,0x00fb                   ;       20 66              .f    
	LD L,C                         ;       69                 i     
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
	LD (HL),E                      ;       73                 s     
	NOP                            ;       00                 .     
L0_9a:
	LD B,(HL)                      ;       46                 F     
L0_9b:
	LD L,C                         ;       69                 i     
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
	JR NZ,0x0114                   ;       20 74              .t    
	LD H,C                         ;       61                 a     
L0_a1:
	LD H,D                         ;       62                 b     
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
	JR NZ,0x0115                   ;       20 6f              .o    
	HALT                           ;       76                 v     
	LD H,L                         ;       65                 e     
	LD (HL),D                      ;       72                 r     
	LD H,(HL)                      ;       66                 f     
	LD L,H                         ;       6c                 l     
	LD L,A                         ;       6f                 o     
	LD (HL),A                      ;       77                 w     
L0_ad:
	NOP                            ;       00                 .     
L0_ae:
	LD C,C                         ; 00ae: 49                 I     
	LD L,(HL)                      ;       6e                 n     
	HALT                           ;       76                 v     
	LD H,C                         ;       61                 a     
	LD L,H                         ;       6c                 l     
	LD L,C                         ;       69                 i     
	LD H,H                         ;       64                 d     
	JR NZ,0x0118                   ; 00b5: 20 61              .a    
	LD (HL),D                      ;       72                 r     
	LD H,A                         ;       67                 g     
	LD (HL),L                      ;       75                 u     
	LD L,L                         ;       6d                 m     
	LD H,L                         ;       65                 e     
	LD L,(HL)                      ;       6e                 n     
	LD (HL),H                      ; 00bd: 74                 t     
	NOP                            ;       00                 .     
L0_bf:
	LD C,C                         ;       49                 I     
	LD (HL),E                      ;       73                 s     
	JR NZ,0x0124                   ;       20 61              .a    
	JR NZ,0x0129                   ;       20 64              .d    
	LD L,C                         ;       69                 i     
	LD (HL),D                      ;       72                 r     
L0_c7:
	LD H,L                         ;       65                 e     
	LD H,E                         ;       63                 c     
	LD (HL),H                      ;       74                 t     
	LD L,A                         ;       6f                 o     
	LD (HL),D                      ;       72                 r     
	LD A,C                         ;       79                 y     
	NOP                            ; 00cd: 00                 .     
L0_ce:
	LD C,(HL)                      ;       4e                 N     
	LD L,A                         ;       6f                 o     
	LD (HL),H                      ;       74                 t     
L0_d1:
	JR NZ,0x0134                   ; 00d1: 20 61              .a    
	JR NZ,0x0139                   ; 00d3: 20 64              .d    
	LD L,C                         ;       69                 i     
	LD (HL),D                      ;       72                 r     
	LD H,L                         ;       65                 e     
	LD H,E                         ;       63                 c     
	LD (HL),H                      ;       74                 t     
	LD L,A                         ;       6f                 o     
	LD (HL),D                      ; 00db: 72                 r     
	LD A,C                         ;       79                 y     
	NOP                            ;       00                 .     
L0_de:
	LD C,(HL)                      ;       4e                 N     
	LD L,A                         ;       6f                 o     
	JR NZ,0x0155                   ;       20 73              .s    
	LD (HL),L                      ;       75                 u     
	LD H,E                         ;       63                 c     
	LD L,B                         ;       68                 h     
	JR NZ,0x014b                   ;       20 64              .d    
	LD H,L                         ;       65                 e     
	HALT                           ;       76                 v     
L0_e9:
	LD L,C                         ;       69                 i     
	LD H,E                         ;       63                 c     
	LD H,L                         ;       65                 e     
	NOP                            ;       00                 .     
L0_ed:
	LD B,E                         ;       43                 C     
	LD (HL),D                      ;       72                 r     
	LD L,A                         ;       6f                 o     
	LD (HL),E                      ; 00f0: 73                 s     
	LD (HL),E                      ;       73                 s     
	DEC L                          ;       2d                 -     
	LD H,H                         ;       64                 d     
	LD H,L                         ;       65                 e     
	HALT                           ;       76                 v     
	LD L,C                         ;       69                 i     
	LD H,E                         ;       63                 c     
	LD H,L                         ; 00f8: 65                 e     
L0_f9:
	JR NZ,0x0167                   ;       20 6c              .l    
	LD L,C                         ; 00fb: 69                 i     
	LD L,(HL)                      ;       6e                 n     
	LD L,E                         ;       6b                 k     
	NOP                            ;       00                 .     
L0_ff:
	LD B,(HL)                      ; 00ff: 46                 F     
	LD L,C                         ;       69                 i     
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
	JR NZ,0x016a                   ;       20 65              .e    
	LD A,B                         ;       78                 x     
L0_106:
	LD L,C                         ;       69                 i     
	LD (HL),E                      ;       73                 s     
	LD (HL),H                      ;       74                 t     
	LD (HL),E                      ;       73                 s     
	NOP                            ;       00                 .     
	LD B,(HL)                      ;       46                 F     
	LD L,C                         ;       69                 i     
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
	JR NZ,0x0180                   ;       20 6f              .o    
	LD (HL),D                      ;       72                 r     
	JR NZ,0x0178                   ;       20 64              .d    
	LD H,L                         ; 0114: 65                 e     
	HALT                           ; 0115: 76                 v     
	LD L,C                         ;       69                 i     
	LD H,E                         ;       63                 c     
	LD H,L                         ; 0118: 65                 e     
L0_119:
	JR NZ,0x017d                   ;       20 62              .b    
	LD (HL),L                      ;       75                 u     
	LD (HL),E                      ;       73                 s     
	LD A,C                         ;       79                 y     
	NOP                            ;       00                 .     
	LD B,D                         ; 011f: 42                 B     
	LD L,H                         ;       6c                 l     
	LD L,A                         ;       6f                 o     
	LD H,E                         ;       63                 c     
	LD L,E                         ;       6b                 k     
	JR NZ,0x018a                   ; 0124: 20 64              .d    
	LD H,L                         ;       65                 e     
	HALT                           ;       76                 v     
	LD L,C                         ;       69                 i     
	LD H,E                         ; 0129: 63                 c     
	LD H,L                         ;       65                 e     
	JR NZ,0x019f                   ;       20 72              .r    
	LD H,L                         ;       65                 e     
	LD (HL),C                      ;       71                 q     
	LD (HL),L                      ;       75                 u     
	LD L,C                         ;       69                 i     
	LD (HL),D                      ;       72                 r     
	LD H,L                         ;       65                 e     
	LD H,H                         ;       64                 d     
	NOP                            ; 0134: 00                 .     
	NOP                            ;       00                 .     
	LD D,B                         ; 0136: 50                 P     
	LD H,L                         ;       65                 e     
	LD (HL),D                      ;       72                 r     
	LD L,L                         ; 0139: 6d                 m     
	LD L,C                         ;       69                 i     
	LD (HL),E                      ;       73                 s     
	LD (HL),E                      ;       73                 s     
	LD L,C                         ;       69                 i     
	LD L,A                         ;       6f                 o     
	LD L,(HL)                      ;       6e                 n     
	JR NZ,0x01a6                   ;       20 64              .d    
	LD H,L                         ;       65                 e     
	LD L,(HL)                      ;       6e                 n     
	LD L,C                         ;       69                 i     
	LD H,L                         ;       65                 e     
	LD H,H                         ;       64                 d     
	NOP                            ;       00                 .     
	LD C,(HL)                      ;       4e                 N     
	LD L,A                         ;       6f                 o     
	LD (HL),H                      ;       74                 t     
	JR NZ,D0_14c                   ; 014b: 20 65              .e    
	LD L,(HL)                      ;       6e                 n     
	LD L,A                         ;       6f                 o     
	LD (HL),L                      ;       75                 u     
	LD H,A                         ;       67                 g     
	LD L,B                         ;       68                 h     
	JR NZ,0x01c1                   ;       20 6d              .m    
	LD H,L                         ;       65                 e     
	LD L,L                         ; 0155: 6d                 m     
	LD L,A                         ;       6f                 o     
	LD (HL),D                      ;       72                 r     
	LD A,C                         ;       79                 y     
	NOP                            ;       00                 .     
	LD C,(HL)                      ; 015a: 4e                 N     
	LD L,A                         ;       6f                 o     
	JR NZ,0x01cb                   ;       20 6d              .m    
	LD L,A                         ;       6f                 o     
	LD (HL),D                      ;       72                 r     
	LD H,L                         ;       65                 e     
	JR NZ,0x01d3                   ;       20 70              .p    
	LD (HL),D                      ;       72                 r     
	LD L,A                         ;       6f                 o     
	LD H,E                         ;       63                 c     
	LD H,L                         ;       65                 e     
	LD (HL),E                      ; 0167: 73                 s     
	LD (HL),E                      ;       73                 s     
	LD H,L                         ;       65                 e     
	LD (HL),E                      ; 016a: 73                 s     
	NOP                            ;       00                 .     
	LD C,(HL)                      ;       4e                 N     
	LD L,A                         ;       6f                 o     
	JR NZ,0x01d3                   ;       20 63              .c    
	LD L,B                         ;       68                 h     
	LD L,C                         ;       69                 i     
	LD L,H                         ;       6c                 l     
	LD H,H                         ;       64                 d     
	LD (HL),D                      ;       72                 r     
	LD H,L                         ;       65                 e     
	LD L,(HL)                      ;       6e                 n     
	NOP                            ;       00                 .     
	LD B,D                         ; 0178: 42                 B     
	LD H,C                         ;       61                 a     
	LD H,H                         ;       64                 d     
	JR NZ,0x01e3                   ;       20 66              .f    
	LD L,C                         ; 017d: 69                 i     
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
	JR NZ,0x01f0                   ; 0180: 20 6e              .n    
	LD (HL),L                      ;       75                 u     
	LD L,L                         ;       6d                 m     
	LD H,D                         ;       62                 b     
	LD H,L                         ;       65                 e     
	LD (HL),D                      ;       72                 r     
	NOP                            ;       00                 .     
	LD B,L                         ;       45                 E     
	LD A,B                         ;       78                 x     
	LD H,L                         ; 018a: 65                 e     
	LD H,E                         ;       63                 c     
	JR NZ,0x01f4                   ;       20 66              .f    
	LD L,A                         ;       6f                 o     
	LD (HL),D                      ;       72                 r     
	LD L,L                         ;       6d                 m     
	LD H,C                         ;       61                 a     
	LD (HL),H                      ;       74                 t     
	JR NZ,0x01fa                   ;       20 65              .e    
	LD (HL),D                      ;       72                 r     
	LD (HL),D                      ;       72                 r     
	LD L,A                         ;       6f                 o     
	LD (HL),D                      ;       72                 r     
	NOP                            ;       00                 .     
	LD B,C                         ; 019a: 41                 A     
	LD (HL),D                      ;       72                 r     
	LD H,A                         ;       67                 g     
	JR NZ,0x020b                   ;       20 6c              .l    
	LD L,C                         ; 019f: 69                 i     
	LD (HL),E                      ;       73                 s     
	LD (HL),H                      ;       74                 t     
	JR NZ,0x0218                   ;       20 74              .t    
	LD L,A                         ;       6f                 o     
	LD L,A                         ;       6f                 o     
	JR NZ,0x0214                   ; 01a6: 20 6c              .l    
	LD L,A                         ;       6f                 o     
	LD L,(HL)                      ;       6e                 n     
	LD H,A                         ;       67                 g     
	NOP                            ;       00                 .     
	LD C,(HL)                      ;       4e                 N     
	LD L,A                         ;       6f                 o     
	JR NZ,0x0223                   ;       20 73              .s    
	LD (HL),L                      ;       75                 u     
	LD H,E                         ;       63                 c     
	LD L,B                         ; 01b2: 68                 h     
	JR NZ,0x0219                   ;       20 64              .d    
	LD H,L                         ;       65                 e     
L0_1b6:
	HALT                           ;       76                 v     
	LD L,C                         ;       69                 i     
	LD H,E                         ;       63                 c     
	LD H,L                         ;       65                 e     
	JR NZ,0x022b                   ;       20 6f              .o    
	LD (HL),D                      ;       72                 r     
	JR NZ,0x0220                   ;       20 61              .a    
	LD H,H                         ;       64                 d     
	LD H,H                         ;       64                 d     
	LD (HL),D                      ; 01c1: 72                 r     
	LD H,L                         ;       65                 e     
	LD (HL),E                      ;       73                 s     
	LD (HL),E                      ;       73                 s     
	NOP                            ;       00                 .     
	LD C,C                         ; 01c6: 49                 I     
	CPL                            ;       2f                 /     
	LD C,A                         ;       4f                 O     
	JR NZ,0x0230                   ;       20 65              .e    
	LD (HL),D                      ; 01cb: 72                 r     
	LD (HL),D                      ;       72                 r     
	LD L,A                         ;       6f                 o     
	LD (HL),D                      ;       72                 r     
	NOP                            ;       00                 .     
	LD C,C                         ;       49                 I     
	LD L,(HL)                      ;       6e                 n     
	LD (HL),H                      ;       74                 t     
	LD H,L                         ; 01d3: 65                 e     
	LD (HL),D                      ;       72                 r     
	LD (HL),D                      ;       72                 r     
	LD (HL),L                      ;       75                 u     
	LD (HL),B                      ;       70                 p     
	LD (HL),H                      ;       74                 t     
	LD H,L                         ;       65                 e     
	LD H,H                         ;       64                 d     
	JR NZ,0x0250                   ;       20 73              .s    
	LD A,C                         ;       79                 y     
	LD (HL),E                      ;       73                 s     
	LD (HL),H                      ;       74                 t     
	LD H,L                         ;       65                 e     
	LD L,L                         ;       6d                 m     
	JR NZ,0x0247                   ;       20 63              .c    
	LD H,C                         ;       61                 a     
	LD L,H                         ;       6c                 l     
	LD L,H                         ;       6c                 l     
	NOP                            ;       00                 .     
	LD C,(HL)                      ; 01e8: 4e                 N     
	LD L,A                         ;       6f                 o     
	JR NZ,0x025f                   ;       20 73              .s    
	LD (HL),L                      ;       75                 u     
	LD H,E                         ;       63                 c     
	LD L,B                         ;       68                 h     
	JR NZ,0x0261                   ;       20 70              .p    
	LD (HL),D                      ;       72                 r     
	LD L,A                         ;       6f                 o     
	LD H,E                         ;       63                 c     
	LD H,L                         ; 01f4: 65                 e     
	LD (HL),E                      ;       73                 s     
	LD (HL),E                      ;       73                 s     
	NOP                            ;       00                 .     
	LD C,(HL)                      ;       4e                 N     
	LD L,A                         ;       6f                 o     
	JR NZ,0x026f                   ; 01fa: 20 73              .s    
	LD (HL),L                      ;       75                 u     
	LD H,E                         ;       63                 c     
	LD L,B                         ;       68                 h     
	JR NZ,0x0267                   ;       20 66              .f    
	LD L,C                         ;       69                 i     
	LD L,H                         ;       6c                 l     
	LD H,L                         ;       65                 e     
	JR NZ,0x0275                   ;       20 6f              .o    
	LD (HL),D                      ;       72                 r     
	JR NZ,0x026d                   ;       20 64              .d    
	LD L,C                         ;       69                 i     
	LD (HL),D                      ;       72                 r     
	LD H,L                         ; 020b: 65                 e     
	LD H,E                         ;       63                 c     
	LD (HL),H                      ;       74                 t     
	LD L,A                         ;       6f                 o     
	LD (HL),D                      ;       72                 r     
	LD A,C                         ;       79                 y     
	NOP                            ;       00                 .     
	LD C,(HL)                      ;       4e                 N     
	LD L,A                         ;       6f                 o     
	LD (HL),H                      ; 0214: 74                 t     
	JR NZ,0x028a                   ;       20 73              .s    
	LD (HL),L                      ;       75                 u     
	LD (HL),B                      ; 0218: 70                 p     
	LD H,L                         ; 0219: 65                 e     
	LD (HL),D                      ;       72                 r     
	DEC L                          ;       2d                 -     
	LD (HL),L                      ;       75                 u     
	LD (HL),E                      ;       73                 s     
	LD H,L                         ;       65                 e     
	LD (HL),D                      ;       72                 r     
	NOP                            ; 0220: 00                 .     
	LD D,L                         ;       55                 U     
	LD L,(HL)                      ;       6e                 n     
	LD L,E                         ; 0223: 6b                 k     
	LD L,(HL)                      ;       6e                 n     
	LD L,A                         ;       6f                 o     
	LD (HL),A                      ;       77                 w     
	LD L,(HL)                      ;       6e                 n     
	JR NZ,0x028f                   ;       20 65              .e    
	LD (HL),D                      ;       72                 r     
	LD (HL),D                      ; 022b: 72                 r     
	LD L,A                         ;       6f                 o     
	LD (HL),D                      ;       72                 r     
	NOP                            ;       00                 .     
	LD A,(BC)                      ; 022f: 0a                 .     
	NOP                            ; 0230: 00                 .     
	LD A,(0x0020)                  ; 0231: 3a 20 00           :..   
_perror:
	CALL c.ent                     ;       cd 00 00           ...   
	PUSH AF                        ;       f5                 .     
	PUSH AF                        ;       f5                 .     
	PUSH AF                        ;       f5                 .     
	PUSH AF                        ;       f5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	OR A,(HL)                      ;       b6                 .     
	JP Z,0x0283                    ;       ca 83 02           ...   
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD A,(HL)                      ;       7e                 ~     
	OR A,A                         ;       b7                 .     
	JP Z,0x0283                    ;       ca 83 02           ...   
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL 0x02fb                    ;       cd fb 02           ...   
	POP AF                         ;       f1                 .     
	PUSH BC                        ;       c5                 .     
	LD HL,0x0004                   ; 025f: 21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	LD HL,0x0002                   ; 0267: 21 02 00           !..   
	PUSH HL                        ;       e5                 .     
	CALL _write                    ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ; 026f: f1                 .     
	POP AF                         ;       f1                 .     
	LD HL,0x0002                   ;       21 02 00           !..   
	PUSH HL                        ;       e5                 .     
	LD HL,0x0231                   ; 0275: 21 31 02           !1.   
	PUSH HL                        ;       e5                 .     
	LD HL,0x0002                   ;       21 02 00           !..   
	PUSH HL                        ;       e5                 .     
	CALL _write                    ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	LD A,(_errno)                  ; 0283: 3a 00 00           :..   
	SUB A,0x01                     ;       d6 01              ..    
	LD A,(_errno)                  ;       3a 01 00           :..   
	SBC A,0x00                     ;       de 00              ..    
	JP M,0x02ad                    ;       fa ad 02           ...   
	LD HL,_errno                   ;       21 00 00           !..   
	LD A,0x20                      ;       3e 20              >.    
	SUB A,(HL)                     ;       96                 .     
	LD A,0x00                      ;       3e 00              >.    
	INC HL                         ;       23                 #     
	SBC A,(HL)                     ;       9e                 .     
	JP M,0x02ad                    ;       fa ad 02           ...   
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(_errno)                  ;       3a 00 00           :..   
	LD (HL),A                      ;       77                 w     
	LD A,(_errno)                  ;       3a 01 00           :..   
	INC HL                         ;       23                 #     
	LD (HL),A                      ;       77                 w     
	JP 0x02b5                      ;       c3 b5 02           ...   
	LD HL,0xfff8                   ; 02ad: 21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	SUB A,A                        ;       97                 .     
	LD (HL),A                      ;       77                 w     
	INC HL                         ;       23                 #     
	LD (HL),A                      ;       77                 w     
	LD HL,0xfff8                   ; 02b5: 21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	ADD HL,HL                      ;       29                 )     
	LD BC,0x0333                   ;       01 33 03           .3.   
	ADD HL,BC                      ;       09                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL 0x02fb                    ;       cd fb 02           ...   
	POP AF                         ;       f1                 .     
	PUSH BC                        ;       c5                 .     
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	ADD HL,HL                      ;       29                 )     
	LD BC,0x0333                   ;       01 33 03           .3.   
	ADD HL,BC                      ;       09                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	LD HL,0x0002                   ;       21 02 00           !..   
	PUSH HL                        ;       e5                 .     
	CALL _write                    ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	LD HL,0x0001                   ;       21 01 00           !..   
	PUSH HL                        ;       e5                 .     
	LD HL,0x022f                   ;       21 2f 02           !/.   
	PUSH HL                        ;       e5                 .     
	LD HL,0x0002                   ;       21 02 00           !..   
	PUSH HL                        ;       e5                 .     
	CALL _write                    ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	JP c.ret                       ;       c3 00 00           ...   
	CALL c.ents                    ; 02fb: cd 00 00           ...   
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (c.r4),HL                   ;       22 00 00           "..   
	LD HL,(c.r4)                   ;       2a 00 00           *..   
	LD (0x0377),HL                 ;       22 77 03           "w.   
	LD HL,(0x0377)                 ; 030f: 2a 77 03           *w.   
	LD A,(HL)                      ;       7e                 ~     
	OR A,A                         ;       b7                 .     
	JP Z,0x0321                    ;       ca 21 03           .!.   
	LD HL,(0x0377)                 ;       2a 77 03           *w.   
	INC HL                         ;       23                 #     
	LD (0x0377),HL                 ;       22 77 03           "w.   
	JP 0x030f                      ;       c3 0f 03           ...   
	LD HL,(0x0377)                 ; 0321: 2a 77 03           *w.   
	PUSH HL                        ;       e5                 .     
	LD HL,c.r4                     ;       21 00 00           !..   
	POP BC                         ;       c1                 .     
	LD A,C                         ;       79                 y     
	SUB A,(HL)                     ;       96                 .     
	LD C,A                         ;       4f                 O     
	LD A,B                         ;       78                 x     
	INC HL                         ;       23                 #     
	SBC A,(HL)                     ;       9e                 .     
	LD B,A                         ;       47                 G     
	JP c.rets                      ;       c3 00 00           ...   
	DB 0x21                        ; 0333: 21                 !     
	DB 0x2                         ;       02                 .     
	DB 0x12                        ;       12                 .     
	DB 0x2                         ;       02                 .     
	DB 0xf8                        ;       f8                 .     
	DB 0x1                         ;       01                 .     
	DB 0xe8                        ;       e8                 .     
	DB 0x1                         ;       01                 .     
	DB 0xd0                        ;       d0                 .     
	DB 0x1                         ;       01                 .     
	DB 0xc6                        ;       c6                 .     
	DB 0x1                         ;       01                 .     
	DB 0xac                        ;       ac                 .     
	DB 0x1                         ;       01                 .     
	DB 0x9a                        ;       9a                 .     
	DB 0x1                         ;       01                 .     
	DB 0x88                        ;       88                 .     
	DB 0x1                         ;       01                 .     
	DB 0x78                        ;       78                 x     
	DB 0x1                         ;       01                 .     
	DB 0x6c                        ;       6c                 l     
	DB 0x1                         ;       01                 .     
	DB 0x5a                        ;       5a                 Z     
	DB 0x1                         ;       01                 .     
	DB 0x48                        ;       48                 H     
	DB 0x1                         ;       01                 .     
	DB 0x36                        ;       36                 6     
	DB 0x1                         ;       01                 .     
	DB 0x35                        ;       35                 5     
	DB 0x1                         ;       01                 .     
	DB 0x1f                        ;       1f                 .     
	DB 0x1                         ;       01                 .     
	DB 0xb                         ;       0b                 .     
	DB 0x1                         ;       01                 .     
	DB 0xff                        ;       ff                 .     
	DB 0x0                         ;       00                 .     
	DB 0xed                        ;       ed                 .     
	DB 0x0                         ;       00                 .     
	DB 0xde                        ;       de                 .     
	DB 0x0                         ;       00                 .     
	DB 0xce                        ;       ce                 .     
	DB 0x0                         ;       00                 .     
	DB 0xbf                        ;       bf                 .     
	DB 0x0                         ;       00                 .     
	DB 0xae                        ;       ae                 .     
	DB 0x0                         ;       00                 .     
	DB 0x9a                        ;       9a                 .     
	DB 0x0                         ;       00                 .     
	DB 0x86                        ;       86                 .     
	DB 0x0                         ;       00                 .     
	DB 0x75                        ;       75                 u     
	DB 0x0                         ;       00                 .     
	DB 0x66                        ;       66                 f     
	DB 0x0                         ;       00                 .     
	DB 0x57                        ;       57                 W     
	DB 0x0                         ;       00                 .     
	DB 0x3f                        ;       3f                 ?     
	DB 0x0                         ;       00                 .     
	DB 0x32                        ;       32                 2     
	DB 0x0                         ;       00                 .     
	DB 0x1c                        ;       1c                 .     
	DB 0x0                         ;       00                 .     
	DB 0xd                         ;       0d                 .     
	DB 0x0                         ;       00                 .     
	DB 0x1                         ;       01                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 0377: 00                 .     
	DB 0x0                         ;       00                 .     
