# z80 disassembly
_execv:
	CALL c.ent                     ; 0000: cd 00 00           ...   
	LD HL,0x0006                   ;       21 06 00           !..   
	ADD HL,DE                      ; 0006: 19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _exec                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	JP c.ret                       ;       c3 00 00           ...   
_execl:
	CALL c.ent                     ;       cd 00 00           ...   
	LD HL,0x0006                   ;       21 06 00           !..   
	ADD HL,DE                      ;       19                 .     
	PUSH HL                        ;       e5                 .     
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _exec                     ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	POP AF                         ;       f1                 .     
	JP c.ret                       ;       c3 00 00           ...   
