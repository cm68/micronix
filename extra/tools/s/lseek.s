# z80 disassembly
_lseek:
	CALL c.ent                     ; 0000: cd 00 00           ...   
	LD HL,0x000a                   ; 0003: 21 0a 00           !..   
	ADD HL,DE                      ; 0006: 19                 .     
	LD A,(HL)                      ;       7e                 ~     
	SUB A,0x03                     ;       d6 03              ..    
	INC HL                         ; 000a: 23                 #     
	LD A,(HL)                      ;       7e                 ~     
	SBC A,0x00                     ;       de 00              ..    
	JP P,0x00b6                    ;       f2 b6 00           ...   
	LD HL,0x0006                   ;       21 06 00           !..   
	ADD HL,DE                      ;       19                 .     
	PUSH HL                        ;       e5                 .     
	POP HL                         ;       e1                 .     
	LD BC,c.r0                     ;       01 00 00           ...   
	CALL c.lcpy                    ;       cd 00 00           ...   
	LD HL,c.r0                     ;       21 00 00           !..   
	PUSH HL                        ;       e5                 .     
	SUB A,A                        ;       97                 .     
L0_22:
	LD (c.r1),A                    ;       32 00 00           2..   
	LD (c.r1),A                    ;       32 01 00           2..   
	LD (c.r1),A                    ;       32 02 00           2..   
L0_2b:
	LD A,0x02                      ;       3e 02              >.    
	LD (c.r1),A                    ;       32 03 00           2..   
	LD HL,c.r1                     ;       21 00 00           !..   
	PUSH HL                        ;       e5                 .     
	CALL c.ldiv                    ;       cd 00 00           ...   
	POP HL                         ;       e1                 .     
	INC HL                         ;       23                 #     
	INC HL                         ;       23                 #     
	LD A,(HL)                      ;       7e                 ~     
	LD (0x00f4),A                  ;       32 f4 00           2..   
	INC HL                         ;       23                 #     
	LD A,(HL)                      ;       7e                 ~     
	LD (0x00f5),A                  ;       32 f5 00           2..   
	LD HL,0x0006                   ;       21 06 00           !..   
	ADD HL,DE                      ;       19                 .     
	PUSH HL                        ;       e5                 .     
	POP HL                         ;       e1                 .     
	LD BC,c.r0                     ;       01 00 00           ...   
	CALL c.lcpy                    ;       cd 00 00           ...   
	LD HL,c.r0                     ;       21 00 00           !..   
	PUSH HL                        ;       e5                 .     
	SUB A,A                        ;       97                 .     
	LD (c.r1),A                    ;       32 00 00           2..   
	LD (c.r1),A                    ;       32 01 00           2..   
	LD (c.r1),A                    ;       32 02 00           2..   
	LD A,0x02                      ;       3e 02              >.    
	LD (c.r1),A                    ;       32 03 00           2..   
	LD HL,c.r1                     ;       21 00 00           !..   
	PUSH HL                        ;       e5                 .     
	CALL c.lmod                    ;       cd 00 00           ...   
	POP HL                         ;       e1                 .     
	INC HL                         ;       23                 #     
	INC HL                         ;       23                 #     
	LD A,(HL)                      ;       7e                 ~     
	LD (0x00f6),A                  ;       32 f6 00           2..   
	INC HL                         ;       23                 #     
	LD A,(HL)                      ;       7e                 ~     
	LD (0x00f7),A                  ;       32 f7 00           2..   
	LD HL,0x000a                   ;       21 0a 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	INC HL                         ;       23                 #     
	INC HL                         ;       23                 #     
	INC HL                         ;       23                 #     
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00f4)                 ;       2a f4 00           *..   
	PUSH HL                        ;       e5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _seek                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	LD A,B                         ;       78                 x     
	OR A,A                         ;       b7                 .     
	JP M,0x00e1                    ;       fa e1 00           ...   
	LD HL,0x0001                   ;       21 01 00           !..   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00f6)                 ;       2a f6 00           *..   
	PUSH HL                        ;       e5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _seek                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	LD A,B                         ;       78                 x     
	OR A,A                         ;       b7                 .     
	JP P,0x00e7                    ;       f2 e7 00           ...   
	JP 0x00e1                      ;       c3 e1 00           ...   
	LD HL,0x000a                   ; 00b6: 21 0a 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	LD HL,0x0006                   ;       21 06 00           !..   
	ADD HL,DE                      ;       19                 .     
	INC HL                         ;       23                 #     
	INC HL                         ;       23                 #     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _seek                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	LD A,B                         ;       78                 x     
	OR A,A                         ;       b7                 .     
	JP P,0x00e7                    ;       f2 e7 00           ...   
	LD BC,0xffff                   ;       01 ff ff           ...   
	JP c.ret                       ;       c3 00 00           ...   
	LD BC,0xffff                   ; 00e1: 01 ff ff           ...   
	JP c.ret                       ;       c3 00 00           ...   
	LD HL,0x0004                   ; 00e7: 21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	JP c.ret                       ;       c3 00 00           ...   
	DB 0x0                         ; 00f4: 00                 .     
	DB 0x0                         ; 00f5: 00                 .     
	DB 0x0                         ; 00f6: 00                 .     
	DB 0x0                         ; 00f7: 00                 .     
