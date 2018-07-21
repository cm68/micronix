# z80 disassembly
_fork:
	SYS FORK                       ; 0000: cf 02              ..    
	JP 0x000f                      ;       c3 0f 00           ...   
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	RET NC                         ;       d0                 .     
	LD BC,0xffff                   ;       01 ff ff           ...   
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
	LD BC,0x0000                   ; 000f: 01 00 00           ...   
	RET                            ;       c9                 .     
