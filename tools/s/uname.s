# z80 disassembly
	CPL                            ; 0000: 2f                 /     
	LD (HL),H                      ; 0001: 74                 t     
	LD L,L                         ; 0002: 6d                 m     
	LD (HL),B                      ; 0003: 70                 p     
	CPL                            ;       2f                 /     
	NOP                            ;       00                 .     
_uname:
	CALL c.ent                     ;       cd 00 00           ...   
	LD HL,0x00c1                   ;       21 c1 00           !..   
	LD A,(HL)                      ;       7e                 ~     
	INC HL                         ;       23                 #     
	OR A,(HL)                      ;       b6                 .     
	INC HL                         ;       23                 #     
	OR A,(HL)                      ;       b6                 .     
	INC HL                         ;       23                 #     
	OR A,(HL)                      ;       b6                 .     
	JP NZ,0x003e                   ;       c2 3e 00           .>.   
	LD HL,0x00c1                   ;       21 c1 00           !..   
	PUSH HL                        ;       e5                 .     
	CALL _time                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	LD HL,0x00c1                   ;       21 c1 00           !..   
	PUSH HL                        ;       e5                 .     
	LD A,0x27                      ;       3e 27              >'    
	ADD A,A                        ;       87                 .     
	SBC A,A                        ;       9f                 .     
	LD (c.r0),A                    ;       32 00 00           2..   
	LD (c.r0),A                    ;       32 01 00           2..   
	LD A,0x27                      ;       3e 27              >'    
	LD (c.r0),A                    ;       32 03 00           2..   
	LD A,0x10                      ;       3e 10              >.    
	LD (c.r0),A                    ;       32 02 00           2..   
	LD HL,c.r0                     ;       21 00 00           !..   
	PUSH HL                        ;       e5                 .     
	CALL c.ulmod                   ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	LD HL,0x0000                   ; 003e: 21 00 00           !..   
	PUSH HL                        ;       e5                 .     
	LD HL,0x0000                   ;       21 00 00           !..   
	PUSH HL                        ;       e5                 .     
	LD HL,0x00af                   ;       21 af 00           !..   
	PUSH HL                        ;       e5                 .     
	CALL _cpystr                   ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	LD L,C                         ;       69                 i     
	LD H,B                         ;       60                 `     
	LD (0x00bf),HL                 ;       22 bf 00           "..   
	LD HL,0x000a                   ;       21 0a 00           !..   
	PUSH HL                        ;       e5                 .     
	CALL _getpid                   ;       cd 00 00           ...   
	PUSH BC                        ;       c5                 .     
	LD HL,(0x00bf)                 ;       2a bf 00           *..   
	PUSH HL                        ;       e5                 .     
	CALL _itob                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	LD HL,(0x00bf)                 ;       2a bf 00           *..   
	ADD HL,BC                      ;       09                 .     
	LD (0x00bf),HL                 ;       22 bf 00           "..   
	LD HL,(0x00bf)                 ;       2a bf 00           *..   
	PUSH HL                        ;       e5                 .     
	LD HL,(0x00bf)                 ;       2a bf 00           *..   
	INC HL                         ;       23                 #     
	LD (0x00bf),HL                 ;       22 bf 00           "..   
	POP HL                         ;       e1                 .     
	LD (HL),0x2d                   ;       36 2d              6-    
	LD HL,0x000a                   ;       21 0a 00           !..   
	PUSH HL                        ;       e5                 .     
	LD HL,0x00c1                   ;       21 c1 00           !..   
	INC HL                         ;       23                 #     
	INC HL                         ;       23                 #     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	DEC HL                         ;       2b                 +     
	DEC HL                         ;       2b                 +     
	DEC HL                         ;       2b                 +     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	LD HL,(0x00bf)                 ;       2a bf 00           *..   
	PUSH HL                        ;       e5                 .     
	CALL _ltob                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	LD HL,(0x00bf)                 ;       2a bf 00           *..   
	ADD HL,BC                      ;       09                 .     
	LD (0x00bf),HL                 ;       22 bf 00           "..   
	LD HL,(0x00bf)                 ;       2a bf 00           *..   
	LD (HL),0x00                   ;       36 00              6.    
	LD HL,0x00af                   ;       21 af 00           !..   
	LD C,L                         ;       4d                 M     
	LD B,H                         ;       44                 D     
	JP c.ret                       ;       c3 00 00           ...   
	DB 0x0                         ; 00af: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DW ‡L@;ã                      ;       00 00              ..    
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00bf: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ; 00c1: 00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
	DB 0x0                         ;       00                 .     
