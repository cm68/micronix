# z80 disassembly
_getpid:
	SYS GETPID                     ;       cf 14              ..    
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	RET                            ;       c9                 .     
