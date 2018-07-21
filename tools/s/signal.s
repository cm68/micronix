# z80 disassembly
_signal:
	CALL c.ents                    ; 0000: cd 00 00           ...   
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ; 0006: 19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (c.r4),HL                   ;       22 00 00           "..   
	LD HL,0x0006                   ;       21 06 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (c.r2),HL                   ;       22 00 00           "..   
	LD A,(c.r4)                    ;       3a 00 00           :..   
	SUB A,0x01                     ;       d6 01              ..    
	LD A,(c.r4)                    ;       3a 01 00           :..   
L0_21:
	SBC A,0x00                     ;       de 00              ..    
	JP M,0x0033                    ;       fa 33 00           .3.   
	LD HL,c.r4                     ;       21 00 00           !..   
	LD A,L0_2a                     ;       3e 0f              >.    
	SUB A,(HL)                     ;       96                 .     
	LD A,0x00                      ;       3e 00              >.    
	INC HL                         ;       23                 #     
	SBC A,(HL)                     ;       9e                 .     
	JP P,0x0039                    ;       f2 39 00           .9.   
	LD BC,0xffff                   ; 0033: 01 ff ff           ...   
	JP c.rets                      ;       c3 00 00           ...   
	LD A,(c.r2)                    ; 0039: 3a 00 00           :..   
	CP A,0x01                      ;       fe 01              ..    
	JP NZ,0x0046                   ;       c2 46 00           .F.   
	LD A,(c.r2)                    ;       3a 01 00           :..   
	CP A,0x00                      ;       fe 00              ..    
	JP Z,0x0058                    ; 0046: ca 58 00           .X.   
	LD HL,c.r2                     ;       21 00 00           !..   
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	OR A,(HL)                      ;       b6                 .     
	JP Z,0x0058                    ;       ca 58 00           .X.   
	LD BC,0x0001                   ;       01 01 00           ...   
	JP 0x005d                      ;       c3 5d 00           .].   
	LD HL,(c.r2)                   ; 0058: 2a 00 00           *..   
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	PUSH BC                        ; 005d: c5                 .     
	LD HL,(c.r4)                   ;       2a 00 00           *..   
	PUSH HL                        ;       e5                 .     
	CALL __signal                  ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	LD L,C                         ;       69                 i     
	LD H,B                         ;       60                 `     
	LD (0x00eb),HL                 ;       22 eb 00           "..   
	LD A,(0x00eb)                  ;       3a eb 00           :..   
	CP A,0x01                      ;       fe 01              ..    
	JP NZ,0x0079                   ;       c2 79 00           .y.   
	LD A,(0x00ec)                  ;       3a ec 00           :..   
	CP A,0x00                      ;       fe 00              ..    
	JP Z,0x0098                    ; 0079: ca 98 00           ...   
	LD HL,0x00eb                   ;       21 eb 00           !..   
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	OR A,(HL)                      ;       b6                 .     
	JP Z,0x0098                    ;       ca 98 00           ...   
	LD HL,(c.r4)                   ;       2a 00 00           *..   
	LD BC,0xffff                   ;       01 ff ff           ...   
	ADD HL,BC                      ;       09                 .     
	ADD HL,HL                      ;       29                 )     
	LD BC,__stab                   ;       01 00 00           ...   
	ADD HL,BC                      ;       09                 .     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x00eb),HL                 ;       22 eb 00           "..   
	LD A,(c.r2)                    ; 0098: 3a 00 00           :..   
	CP A,0x01                      ;       fe 01              ..    
	JP NZ,0x00a5                   ;       c2 a5 00           ...   
	LD A,(c.r2)                    ;       3a 01 00           :..   
	CP A,0x00                      ;       fe 00              ..    
	JP Z,0x00e3                    ; 00a5: ca e3 00           ...   
	LD HL,c.r2                     ;       21 00 00           !..   
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	OR A,(HL)                      ;       b6                 .     
	JP Z,0x00e3                    ;       ca e3 00           ...   
	LD HL,(c.r4)                   ;       2a 00 00           *..   
	LD BC,0xffff                   ;       01 ff ff           ...   
	ADD HL,BC                      ;       09                 .     
	ADD HL,HL                      ;       29                 )     
	LD BC,__stab                   ;       01 00 00           ...   
	ADD HL,BC                      ;       09                 .     
	LD A,(c.r2)                    ;       3a 00 00           :..   
	LD (HL),A                      ;       77                 w     
	LD A,(c.r2)                    ;       3a 01 00           :..   
	INC HL                         ;       23                 #     
	LD (HL),A                      ;       77                 w     
	LD HL,(c.r4)                   ;       2a 00 00           *..   
	LD BC,0xffff                   ;       01 ff ff           ...   
	ADD HL,BC                      ;       09                 .     
	PUSH BC                        ;       c5                 .     
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	ADD HL,HL                      ;       29                 )     
	ADD HL,BC                      ;       09                 .     
	ADD HL,HL                      ;       29                 )     
	ADD HL,BC                      ;       09                 .     
	POP BC                         ;       c1                 .     
	LD BC,__jtab                   ;       01 00 00           ...   
	ADD HL,BC                      ;       09                 .     
	PUSH HL                        ;       e5                 .     
	LD HL,(c.r4)                   ;       2a 00 00           *..   
	PUSH HL                        ;       e5                 .     
	CALL __signal                  ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	LD HL,(0x00eb)                 ; 00e3: 2a eb 00           *..   
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	JP c.rets                      ;       c3 00 00           ...   
	DB 0x0                         ; 00eb: 00                 .     
	DB 0x0                         ; 00ec: 00                 .     
