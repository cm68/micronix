# z80 disassembly
_getuid:
	SYS GETUID                     ;       cf 18              ..    
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	RET                            ;       c9                 .     
