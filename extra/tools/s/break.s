# z80 disassembly
__break:
	POP BC                         ; 0000: c1                 .     
	POP HL                         ;       e1                 .     
	PUSH HL                        ;       e5                 .     
	PUSH BC                        ;       c5                 .     
	LD (D0_5),HL                   ;       22 16 00           "..   
	SYS INDIR 0x0014               ;       cf 00 14 00        ....  
	LD BC,0x0000                   ;       01 00 00           ...   
	RET NC                         ;       d0                 .     
	DEC BC                         ;       0b                 .     
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 0014: cf                 .     
	DB 0x11                        ;       11                 .     
	DB 0x0                         ; 0016: 00                 .     
	DB 0x0                         ;       00                 .     
