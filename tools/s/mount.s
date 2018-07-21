# z80 disassembly
_mount:
	LD HL,0x0002                   ; 0000: 21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ; 0004: 7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ; 0006: 66                 f     
	LD L,A                         ;       6f                 o     
	LD (D0_9),HL                   ;       22 30 00           "0.   
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0032),HL                 ;       22 32 00           "2.   
	LD HL,0x0006                   ;       21 06 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0034),HL                 ;       22 34 00           "4.   
	SYS INDIR 0x002e               ;       cf 00 2e 00        ....  
	LD BC,0x0000                   ;       01 00 00           ...   
	RET NC                         ;       d0                 .     
	DEC BC                         ;       0b                 .     
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 002e: cf                 .     
	DW ÀK@«¡                      ;       15 00              ..    
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 0032: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 0034: 00                 .     
	DB 0x0                         ;       00                 .     
