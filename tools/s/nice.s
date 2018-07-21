# z80 disassembly
_nice:
	LD HL,0x0002                   ; 0000: 21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	SYS NICE                       ;       cf 22              ."    
	LD BC,0x0000                   ;       01 00 00           ...   
	RET NC                         ;       d0                 .     
	DEC BC                         ;       0b                 .     
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
