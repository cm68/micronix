# z80 disassembly
_stime:
	PUSH DE                        ; 0000: d5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,SP                      ; 0004: 39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD E,(HL)                      ;       5e                 ^     
	INC HL                         ;       23                 #     
	LD D,(HL)                      ;       56                 V     
	INC HL                         ;       23                 #     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	EX DE,HL                       ;       eb                 .     
	SYS STIME                      ;       cf 19              ..    
	POP DE                         ;       d1                 .     
	LD BC,0x0000                   ;       01 00 00           ...   
	RET NC                         ;       d0                 .     
	DEC BC                         ;       0b                 .     
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
