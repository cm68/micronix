# z80 disassembly
_write:
	LD HL,0x0004                   ; 0000: 21 04 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ; 0004: 7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ; 0006: 66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x002e),HL                 ;       22 2e 00           "..   
	LD HL,0x0006                   ;       21 06 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0030),HL                 ;       22 30 00           "0.   
	LD HL,0x0002                   ;       21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	SYS INDIR 0x002c               ;       cf 00 2c 00        ..,.  
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	RET NC                         ;       d0                 .     
	LD BC,0xffff                   ;       01 ff ff           ...   
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 002c: cf                 .     
	DW ÀK@ƒò                      ;       04 00              ..    
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 0030: 00                 .     
	DB 0x0                         ;       00                 .     
