# z80 disassembly
_exec:
	LD HL,0x0002                   ; 0000: 21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ; 0004: 7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0023),HL                 ;       22 23 00           "#.   
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0025),HL                 ;       22 25 00           "%.   
	SYS INDIR 0x0021               ;       cf 00 21 00        ..!.  
	LD BC,0xffff                   ;       01 ff ff           ...   
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 0021: cf                 .     
	DB 0xb                         ;       0b                 .     
	DB 0x0                         ; 0023: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 0025: 00                 .     
	DB 0x0                         ;       00                 .     
