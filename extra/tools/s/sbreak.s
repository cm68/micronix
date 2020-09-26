# z80 disassembly
_sbreak:
	CALL L0_1                      ; 0000: cd 00 00           ...   
	PUSH AF                        ;       f5                 .     
	PUSH AF                        ; 0004: f5                 .     
	PUSH AF                        ;       f5                 .     
	PUSH AF                        ;       f5                 .     
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	PUSH HL                        ;       e5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL 0x0042                    ;       cd 42 00           .B.   
	POP AF                         ;       f1                 .     
	POP HL                         ;       e1                 .     
	LD A,C                         ;       79                 y     
	LD (HL),A                      ;       77                 w     
	LD A,B                         ;       78                 x     
	INC HL                         ;       23                 #     
	LD (HL),A                      ;       77                 w     
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	CP A,0xff                      ;       fe ff              ..    
	JP NZ,0x002c                   ;       c2 2c 00           .,.   
	INC HL                         ;       23                 #     
	LD A,(HL)                      ;       7e                 ~     
	CP A,0xff                      ;       fe ff              ..    
	JP NZ,0x0035                   ; 002c: c2 35 00           .5.   
	LD BC,0x0000                   ;       01 00 00           ...   
	JP c.ret                       ;       c3 00 00           ...   
	LD HL,0xfff8                   ; 0035: 21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	JP c.ret                       ;       c3 00 00           ...   
_sbrk:
	CALL c.ent                     ; 0042: cd 00 00           ...   
	LD HL,0xfff6                   ;       21 f6 ff           !..   
	ADD HL,SP                      ;       39                 9     
	LD SP,HL                       ;       f9                 .     
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(0x0107)                  ;       3a 07 01           :..   
	LD (HL),A                      ;       77                 w     
	LD A,(0x0108)                  ;       3a 08 01           :..   
	INC HL                         ;       23                 #     
	LD (HL),A                      ;       77                 w     
	LD HL,0xfff6                   ;       21 f6 ff           !..   
	ADD HL,DE                      ;       19                 .     
	PUSH HL                        ;       e5                 .     
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	PUSH HL                        ;       e5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	EX (SP),HL                     ;       e3                 .     
	POP BC                         ;       c1                 .     
	ADD HL,BC                      ;       09                 .     
	POP BC                         ;       c1                 .     
	LD A,L                         ;       7d                 }     
	LD (BC),A                      ;       02                 .     
	LD A,H                         ;       7c                 |     
	INC BC                         ;       03                 .     
	LD (BC),A                      ;       02                 .     
	LD HL,0xfff6                   ;       21 f6 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL __break                   ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	LD A,C                         ;       79                 y     
	CP A,0xff                      ;       fe ff              ..    
	JP NZ,0x008b                   ;       c2 8b 00           ...   
	LD A,B                         ;       78                 x     
	CP A,0xff                      ;       fe ff              ..    
	JP NZ,0x0094                   ; 008b: c2 94 00           ...   
	LD BC,0xffff                   ;       01 ff ff           ...   
	JP c.ret                       ;       c3 00 00           ...   
	LD HL,0xfff6                   ; 0094: 21 f6 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0107),HL                 ;       22 07 01           "..   
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	JP c.ret                       ;       c3 00 00           ...   
_brk:
	CALL c.ent                     ;       cd 00 00           ...   
	LD HL,0xfff6                   ;       21 f6 ff           !..   
	ADD HL,SP                      ;       39                 9     
	LD SP,HL                       ;       f9                 .     
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(0x0107)                  ;       3a 07 01           :..   
	LD (HL),A                      ;       77                 w     
	LD A,(0x0108)                  ;       3a 08 01           :..   
	INC HL                         ;       23                 #     
	LD (HL),A                      ;       77                 w     
	LD HL,0xfff6                   ;       21 f6 ff           !..   
	ADD HL,DE                      ;       19                 .     
	PUSH HL                        ;       e5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	POP BC                         ;       c1                 .     
	LD A,(HL)                      ;       7e                 ~     
	LD (BC),A                      ;       02                 .     
	INC HL                         ;       23                 #     
	LD A,(HL)                      ;       7e                 ~     
	INC BC                         ;       03                 .     
	LD (BC),A                      ;       02                 .     
	LD HL,0xfff6                   ;       21 f6 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL __break                   ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	LD A,C                         ;       79                 y     
	CP A,0xff                      ;       fe ff              ..    
	JP NZ,0x00e6                   ;       c2 e6 00           ...   
	LD A,B                         ;       78                 x     
	CP A,0xff                      ;       fe ff              ..    
	JP NZ,0x00ef                   ; 00e6: c2 ef 00           ...   
	LD BC,0xffff                   ;       01 ff ff           ...   
	JP c.ret                       ;       c3 00 00           ...   
	LD HL,0xfff6                   ; 00ef: 21 f6 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0107),HL                 ;       22 07 01           "..   
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
L0_100:
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	JP c.ret                       ;       c3 00 00           ...   
	DW                             ; 0107: 00 00              ..    
