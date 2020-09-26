# z80 disassembly
_wait:
	PUSH DE                        ; 0000: d5                 .     
	SYS WAIT                       ;       cf 07              ..    
	JP C,0x0015                    ;       da 15 00           ...   
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (HL),E                      ;       73                 s     
	INC HL                         ;       23                 #     
	LD (HL),D                      ;       72                 r     
	POP DE                         ;       d1                 .     
	RET                            ;       c9                 .     
	POP DE                         ; 0015: d1                 .     
	LD BC,0xffff                   ;       01 ff ff           ...   
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
