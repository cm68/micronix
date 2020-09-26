# z80 disassembly
_creat:
	LD HL,0x0002                   ; 0000: 21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ; 0004: 7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0026),HL                 ;       22 26 00           "&.   
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x0028),HL                 ;       22 28 00           "(.   
	SYS INDIR 0x0024               ;       cf 00 24 00        ..$.  
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	RET NC                         ;       d0                 .     
	LD BC,0xffff                   ;       01 ff ff           ...   
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
	DB 0xcf                        ; 0024: cf                 .     
	DB 0x8                         ;       08                 .     
	DB 0x0                         ; 0026: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 0028: 00                 .     
	DB 0x0                         ;       00                 .     
