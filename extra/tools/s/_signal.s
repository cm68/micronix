# z80 disassembly
__signal:
	LD HL,0x0002                   ; 0000: 21 02 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ; 0004: 7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x009a),HL                 ;       22 9a 00           "..   
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,SP                      ;       39                 9     
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	LD H,(HL)                      ;       66                 f     
	LD L,A                         ;       6f                 o     
	LD (0x009c),HL                 ;       22 9c 00           "..   
	SYS INDIR 0x0098               ;       cf 00 98 00        ....  
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	RET NC                         ;       d0                 .     
	LD BC,0xffff                   ;       01 ff ff           ...   
	LD (_errno),HL                 ;       22 00 00           "..   
	RET                            ;       c9                 .     
__jtab:
	PUSH HL                        ;       e5                 .     
	LD HL,(0x009e)                 ;       2a 9e 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00a0)                 ;       2a a0 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00a2)                 ;       2a a2 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00a4)                 ;       2a a4 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00a6)                 ;       2a a6 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(D0_49)                  ;       2a a8 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00aa)                 ;       2a aa 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00ac)                 ;       2a ac 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00ae)                 ;       2a ae 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00b0)                 ;       2a b0 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00b2)                 ;       2a b2 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00b4)                 ;       2a b4 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00b6)                 ;       2a b6 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00b8)                 ;       2a b8 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00ba)                 ;       2a ba 00           *..   
	JP 0x008d                      ;       c3 8d 00           ...   
	PUSH DE                        ; 008d: d5                 .     
	PUSH BC                        ;       c5                 .     
	PUSH AF                        ;       f5                 .     
	CALL c.ihl                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP BC                         ;       c1                 .     
	POP DE                         ;       d1                 .     
	POP HL                         ;       e1                 .     
	RET                            ;       c9                 .     
	DB 0xcf                        ; 0098: cf                 .     
	DB 0x30                        ;       30                 0     
	DB 0x0                         ; 009a: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 009c: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 009e: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00a0: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00a2: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00a4: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00a6: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00a8: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00aa: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00ac: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00ae: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00b0: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00b2: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00b4: 00                 .     
	DW °LÀ÷ä                      ;       00 00              ..    
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00b8: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00ba: 00                 .     
	DB 0x0                         ;       00                 .     
