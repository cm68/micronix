# z80 disassembly
_close:
	POP BC                         ; 0000: c1                 .     
	POP HL                         ;       e1                 .     
	PUSH HL                        ;       e5                 .     
	PUSH BC                        ;       c5                 .     
	SYS CLOSE                      ;       cf 06              ..    
	LD BC,0x0000                   ;       01 00 00           ...   
	RET NC                         ;       d0                 .     
	DEC BC                         ;       0b                 .     
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
