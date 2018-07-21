# z80 disassembly
_stty:
	LD HL,0x0004                   ; 0000: 21 04 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ; 0004: 7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0022),HL                 ;       22 22 00           "".   
	LD HL,0x0002                   ;       21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	SYS INDIR 0x0020               ;       cf 00 20 00        ....  
	LD BC,0x0000                   ;       01 00 00           ...   
	RET NC                         ;       d0                 .     
	DEC BC                         ;       0b                 .     
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 0020: cf                 .     
	DB 0x1f                        ;       1f                 .     
	DB 0x0                         ; 0022: 00                 .     
	DB 0x0                         ;       00                 .     
