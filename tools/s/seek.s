# z80 disassembly
_seek:
	LD HL,0x0004                   ; 0000: 21 04 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ; 0004: 7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ; 0006: 66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x002d),HL                 ;       22 2d 00           "-.   
	LD HL,0x0006                   ;       21 06 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x002f),HL                 ;       22 2f 00           "/.   
	LD HL,0x0002                   ;       21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	SYS INDIR 0x002b               ;       cf 00 2b 00        ..+.  
	LD BC,0x0000                   ;       01 00 00           ...   
	RET NC                         ;       d0                 .     
	DEC BC                         ;       0b                 .     
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 002b: cf                 .     
	DB 0x13                        ;       13                 .     
	DW ÀKÀ™Þ                      ; 002d: 00 00              ..    
	DB 0x0                         ; 002f: 00                 .     
	DB 0x0                         ;       00                 .     
