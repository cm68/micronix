# z80 disassembly
_pipe:
	PUSH DE                        ; 0000: d5                 .     
	SYS PIPE                       ;       cf 2a              .*    
	EX DE,HL                       ;       eb                 .     
	PUSH HL                        ;       e5                 .     
	LD HL,0x0006                   ;       21 06 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (HL),E                      ;       73                 s     
	INC HL                         ;       23                 #     
	LD (HL),D                      ;       72                 r     
	INC HL                         ;       23                 #     
	POP DE                         ;       d1                 .     
	LD (HL),E                      ;       73                 s     
	INC HL                         ;       23                 #     
	LD (HL),D                      ;       72                 r     
	POP DE                         ;       d1                 .     
	LD BC,0x0000                   ;       01 00 00           ...   
	RET NC                         ;       d0                 .     
	DEC BC                         ;       0b                 .     
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
