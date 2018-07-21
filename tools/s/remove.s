# z80 disassembly
_remove:
	CALL c.ent                     ; 0000: cd 00 00           ...   
	LD HL,0x0004                   ;       21 04 00           !..   
	ADD HL,DE                      ;       19                 .     
	LD C,(HL)                      ;       4e                 N     
	INC HL                         ;       23                 #     
	LD B,(HL)                      ;       46                 F     
	PUSH BC                        ;       c5                 .     
	CALL _unlink                   ;       cd 00 00           ...   
	POP AF                         ;       f1                 .     
	JP c.ret                       ;       c3 00 00           ...   
