# z80 disassembly
_chmod:
	LD HL,0x0002                   ; 0000: 21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ; 0004: 7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0025),HL                 ;       22 25 00           "%.   
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0027),HL                 ;       22 27 00           "'.   
	SYS INDIR 0x0023               ;       cf 00 23 00        ..#.  
	LD BC,0x0000                   ;       01 00 00           ...   
	RET NC                         ;       d0                 .     
	DEC BC                         ;       0b                 .     
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 0023: cf                 .     
	DB 0xf                         ;       0f                 .     
	DB 0x0                         ; 0025: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 0027: 00                 .     
	DB 0x0                         ;       00                 .     
