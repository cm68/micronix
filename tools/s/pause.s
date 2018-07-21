# z80 disassembly
_pause:
	SYS INDIR 0x0008               ; 0000: cf 00 08 00        ....  
	LD BC,0x0000                   ;       01 00 00           ...   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 0008: cf                 .     
	DB 0x1d                        ;       1d                 .     
