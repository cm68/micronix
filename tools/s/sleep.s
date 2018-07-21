# z80 disassembly
_sleep:
	LD HL,0x0002                   ; 0000: 21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	SYS SLEEP                      ;       cf 23              .#    
	LD BC,0x0000                   ;       01 00 00           ...   
	RET                            ;       c9                 .     
