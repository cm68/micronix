# z80 disassembly
_create:
	CALL c.ent                     ; 0000: cd 00 00           ...   
	PUSH AF                        ;       f5                 .     
	PUSH AF                        ; 0004: f5                 .     
	PUSH AF                        ;       f5                 .     
	PUSH AF                        ; 0006: f5                 .     
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	PUSH HL                        ;       e5                 .     
	LD HL,0x01ff                   ;       21 ff 01           !..   
	PUSH HL                        ;       e5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _creat                    ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP HL                         ;       e1                 .     
	LD A,C                         ;       79                 y     
	LD (HL),A                      ;       77                 w     
	LD A,B                         ;       78                 x     
	INC HL                         ;       23                 #     
	LD (HL),A                      ;       77                 w     
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
L0_27:
	INC HL                         ;       23                 #     
	LD A,(HL)                      ;       7e                 ~     
	OR A,A                         ;       b7                 .     
	JP P,0x003a                    ;       f2 3a 00           .:.   
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	JP c.ret                       ;       c3 00 00           ...   
	LD HL,0x0006                   ; 003a: 21 06 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	OR A,(HL)                      ;       b6                 .     
	JP NZ,0x006f                   ;       c2 6f 00           .o.   
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _close                    ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	PUSH HL                        ;       e5                 .     
	LD HL,0x0000                   ;       21 00 00           !..   
	PUSH HL                        ;       e5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _open                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP HL                         ;       e1                 .     
	LD A,C                         ;       79                 y     
	LD (HL),A                      ;       77                 w     
	LD A,B                         ;       78                 x     
	INC HL                         ;       23                 #     
	LD (HL),A                      ;       77                 w     
	JP 0x00a8                      ;       c3 a8 00           ...   
	LD HL,0x0006                   ; 006f: 21 06 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	CP A,0x02                      ;       fe 02              ..    
	JP NZ,0x007d                   ;       c2 7d 00           .}.   
	INC HL                         ;       23                 #     
	LD A,(HL)                      ;       7e                 ~     
	CP A,0x00                      ;       fe 00              ..    
	JP NZ,0x00a8                   ; 007d: c2 a8 00           ...   
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _close                    ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	LD HL,0xfff8                   ;       21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	PUSH HL                        ;       e5                 .     
	LD HL,0x0002                   ;       21 02 00           !..   
	PUSH HL                        ;       e5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _open                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
L0_a1:
	POP AF                         ;       f1                 .     
	POP HL                         ;       e1                 .     
	LD A,C                         ;       79                 y     
	LD (HL),A                      ;       77                 w     
	LD A,B                         ;       78                 x     
	INC HL                         ;       23                 #     
	LD (HL),A                      ;       77                 w     
	LD HL,0xfff8                   ; 00a8: 21 f8 ff           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	JP c.ret                       ;       c3 00 00           ...   
