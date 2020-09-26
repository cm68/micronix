# z80 disassembly
__exit:
	POP BC                         ;       c1                 .     
	POP HL                         ;       e1                 .     
	PUSH HL                        ;       e5                 .     
	PUSH BC                        ;       c5                 .     
	SYS EXIT                       ;       cf 01              ..    
