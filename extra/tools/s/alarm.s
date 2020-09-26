# z80 disassembly
_alarm:
	LD HL,0x0002                   ; 0000: 21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	SYS INDIR 0x0010               ;       cf 00 10 00        ....  
	LD BC,0x0000                   ;       01 00 00           ...   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 0010: cf                 .     
	DB 0x1b                        ;       1b                 .     
